#include "calculator.hpp"

namespace calc {
double IdentifierAST::eval(Calculator *C) { return C->getValue(getName()); }

double BinExprAST::eval(Calculator *C) {
  if (Op == '=') {
    double Ret;
    C->setValue(dynamic_cast<const IdentifierAST &>(*LHS).getName(),
                (Ret = RHS->eval(C)));
    return Ret;
  }

  auto L = LHS->eval(C);
  auto R = RHS->eval(C);

  switch (Op) {
  case '+':
    return L + R;
  case '-':
    return L - R;
  case '*':
    return L * R;
  case '/':
    return L / R;
  case '^':
    return std::pow(L, R);
  default:
    assert(0 && "Unknown operator");
  }
}

double CallExprAST::eval(Calculator *C) {
  std::vector<double> ArgV;
  ArgV.reserve(Args.size());
  for (auto &&X : Args)
    ArgV.push_back(X->eval(C));
  return C->getFunction(Name)(std::move(ArgV));
}

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

double Calculator::calculate(std::string Expr) noexcept {
  Calculation C{std::istringstream(std::move(Expr))};
  auto Ast = C();
  return Ast->eval(this);
}

} // namespace calc