#pragma once
#include <cassert>
#include <cmath>
#include <functional>
#include <future>
#include <map>
#include <memory>
#include <optional>
#include <sstream>
#include <string_view>
#include <variant>

namespace calc {

enum TokenKind { TK_None = 0, TK_String = -1, TK_Number = -2, TK_END = -3 };

struct Token {
  int Kind;

  std::string Str{};

  bool operator==(std::string_view RHS) const noexcept {
    return Kind == TK_String && Str == RHS;
  }
  bool operator==(int RHS) const noexcept { return Kind == RHS; }

  double toNumber() const noexcept {
    std::istringstream SS(Str);
    double N;
    SS >> N;
    return N;
  }

  std::string getDescription() const {
    if (Kind == TK_String)
      return Str;
    if (Kind == TK_Number)
      return Str;
    if (Kind > 0)
      return std::string() + static_cast<char>(Kind);
    if (Kind == TK_END)
      return "<END>";
    return "<Err>";
  }
};

class Calculator;

class AST {
public:
  virtual double eval(Calculator *) = 0;
};

class IdentifierAST : public AST {
  std::string Name;

public:
  explicit IdentifierAST(std::string Name) : Name(std::move(Name)) {}

  double eval(Calculator *C) final;
  const std::string &getName() const & { return Name; }
};

class UnaryExprAST : public AST {
  std::unique_ptr<AST> Operand;
  char Op;

public:
  UnaryExprAST(std::unique_ptr<AST> Operand, char Op)
      : Operand(std::move(Operand)), Op(Op) {}

  double eval(Calculator *C) final {
    if (Op == '-') {
      return -Operand->eval(C);
    }
    assert(0 && "Unknown operator");
  }
};

class BinExprAST : public AST {
  std::unique_ptr<AST> LHS, RHS;
  char Op;

public:
  BinExprAST(std::unique_ptr<AST> LHS, std::unique_ptr<AST> RHS, char Op)
      : LHS(std::move(LHS)), RHS(std::move(RHS)), Op(Op) {}

  double eval(Calculator *C) final;
};

class ConstExpr : public AST {
  double Value;

public:
  explicit ConstExpr(double Value) : Value(Value) {}

  double eval(Calculator *C) final { return Value; }
};

class CallExprAST : public AST {
  std::string Name;
  std::vector<std::unique_ptr<AST>> Args;

public:
  CallExprAST(std::string Name, std::vector<std::unique_ptr<AST>> Args)
      : Name(std::move(Name)), Args(std::move(Args)) {}

  double eval(Calculator *C) final;
};

struct Calculation {
  using result_type = double;

  std::istringstream SS;

  Token CurrentToken = {0, {}};

  Token parseToken() {
    int C;
    while (std::isspace((C = SS.get())))
      ;

    if (std::isalpha(C)) {
      std::string S;
      S.push_back(C);
      while (std::isalnum((C = SS.get()))) {
        S.push_back(C);
      }
      SS.unget();
      return {TK_String, std::move(S)};
    }

    if (std::isdigit(C) || C == '.') {
      std::string S;
      do {
        if (C == '-' || C == '+') {
          if (S.empty() || (S.back() != 'e' && S.back() != 'E')) {
            break;
          }
        }

        if (C == '.' && S.find('.') < S.length())
          break;
        S.push_back(C);
        C = SS.get();
      } while (std::isdigit(C) || C == '.' || C == 'e' || C == 'E' ||
               C == '-' || C == '+');
      SS.unget();
      return {TK_Number, std::move(S)};
    }

    if (C == EOF || C == '\n') {
      SS.unget();
      return {TK_END};
    }

    return {C};
  }

  Token peekToken() {
    if (CurrentToken == 0)
      CurrentToken = parseToken();
    return CurrentToken;
  }

  void eatToken() { CurrentToken = parseToken(); }

  std::unique_ptr<AST> parseExpr() {
    auto LHS = parsePoly();

    if (auto Op = peekToken(); Op == '=') {
      eatToken();
      auto RHS = parseExpr();
      return std::make_unique<BinExprAST>(std::move(LHS), std::move(RHS), '=');
    }
    return LHS;
  }

  std::unique_ptr<AST> parsePoly() {
    auto V = parseTerm();

    while (true) {
      if (auto Op = peekToken(); Op == '+' || Op == '-') {
        eatToken();
        auto V2 = parseTerm();
        V = std::make_unique<BinExprAST>(std::move(V), std::move(V2), Op.Kind);
        continue;
      }
      return V;
    }
  }

  std::unique_ptr<AST> parseTerm() {
    auto V = parsePower();
    while (true) {
      if (auto Op = peekToken(); Op == '*' || Op == '/') {
        eatToken();
        auto V2 = parsePower();
        V = std::make_unique<BinExprAST>(std::move(V), std::move(V2), Op.Kind);
        continue;
      }
      return V;
    }
  }

  std::unique_ptr<AST> parsePower() {
    auto V = parseParen();
    if (auto Op = peekToken(); Op == '^') {
      eatToken();
      return std::make_unique<BinExprAST>(std::move(V), parsePower(), '^');
    }
    return V;
  }

  std::unique_ptr<AST> parseParen() {
    if (auto Tok = peekToken(); Tok == '(') {
      eatToken();
      auto V = parseExpr();
      Tok = peekToken();
      assert(Tok == ')');
      eatToken();
      return V;
    }
    return parsePrimary();
  }

  std::unique_ptr<AST> parsePrimary() {
    auto UnaryOp = peekToken();
    if (UnaryOp == '-' || UnaryOp == '+')
      eatToken();
    else
      UnaryOp.Kind = 0;

    auto Tok = peekToken();
    eatToken();

    std::unique_ptr<AST> Ret;

    if (Tok == TK_Number)
      Ret = std::make_unique<ConstExpr>(Tok.toNumber());
    else if (Tok == TK_String) {
      Ret = std::make_unique<IdentifierAST>(Tok.Str);
      if (peekToken() == '(') {
        eatToken();
        auto Args = parseArgList();
        assert(peekToken() == ')');
        eatToken();
        Ret = std::make_unique<CallExprAST>(
            dynamic_cast<IdentifierAST const &>(*Ret).getName(),
            std::move(Args));
      }
    } else
      throw CalculationError("Expected number, but got " +
                             Tok.getDescription());

    if (UnaryOp == 0)
      return Ret;
    return std::make_unique<UnaryExprAST>(std::move(Ret), UnaryOp.Kind);
  }

  std::vector<std::unique_ptr<AST>> parseArgList() {
    std::vector<std::unique_ptr<AST>> Ret;
    auto Tok = peekToken();
    if (Tok == ')')
      return Ret;
    while (true) {
      Ret.push_back(parseExpr());
      if (peekToken() == ')')
        return Ret;
      if (peekToken() == ',')
        eatToken();
      else
        assert(0 && "unknown token");
    }
  }

  std::unique_ptr<AST> operator()() {
    auto V = parseExpr();
    if (peekToken() == TK_END) {
      return V;
    }
    throw CalculationError("Unexpected trailing tokens " +
                           peekToken().getDescription());
  }

  class CalculationError : public std::exception {
    std::string Msg;

  public:
    CalculationError(std::string Msg) : Msg(std::move(Msg)) {}

    const char *what() const noexcept { return Msg.c_str(); }
  };
};

class Calculator {
  std::map<std::string, double> Variables;
  std::map<std::string, std::function<double(std::vector<double>)>> Functions;

public:
  double calculate(std::string Expr) noexcept {
    Calculation C{std::istringstream(std::move(Expr))};
    auto Ast = C();
    return Ast->eval(this);
  }

  double getValue(const std::string &Name) const { return Variables.at(Name); }

  void setValue(const std::string &Name, double Value) {
    Variables[Name] = Value;
  }

  std::function<double(std::vector<double>)>
  getFunction(const std::string &Name) const & {
    return Functions.at(Name);
  }

  template <typename F> auto setFunction(const std::string &Name, F &&Func) {
    Functions[Name] = std::forward<F>(Func);
  }
};
} // namespace calc
