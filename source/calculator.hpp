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
  virtual ~AST() = default;
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

class Calculator {
  std::map<std::string, double> Variables;
  std::map<std::string, std::function<double(std::vector<double>)>> Functions;

public:
  double calculate(std::string Expr) noexcept;

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
