#include "calculator.hpp"

#include <set>

namespace calc {
double IdentifierAST::eval(Calculator *C) { return C->getValue(getName()); }

std::vector<std::string> CallExprAST::getParams() const {
  std::vector<std::string> Ret;
  Ret.reserve(Args.size());
  for (auto &&X : Args) {
    Ret.emplace_back(dynamic_cast<const IdentifierAST &>(*X).getName());
  }
  return Ret;
}

double BinExprAST::eval(Calculator *C) {
  if (Op == '=') {
    if (const auto Identifier =
            dynamic_cast<const IdentifierAST *>(LHS.get())) {
      double Ret;
      C->setValue(Identifier->getName(), (Ret = RHS->eval(C)));
      return Ret;
    }
    if (const auto Function = dynamic_cast<const CallExprAST *>(LHS.get())) {
      const auto Params =
          std::make_shared<const decltype(Function->getParams())>(
              Function->getParams());
      const std::shared_ptr Body(std::move(RHS));

      auto F = [Params, Body](Calculator *C, std::vector<double> Args) {
        const auto _ = C->createScope();
        const auto N = Params->size();
        for (size_t I = 0; I != N; ++I) {
          C->setLocalVar(Params->at(I), Args[I]);
        }
        return Body->eval(C);
      };

      C->setFunction(Function->getName(), std::move(F));
      return 0;
    }

    throw CalculationError("Syntax Error ");
  }

  const auto L = LHS->eval(C);
  const auto R = RHS->eval(C);

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
    throw CalculationError(std::string("Unknown operator: ") + Op);
  }
}

double CallExprAST::eval(Calculator *C) {
  std::vector<double> ArgV;
  ArgV.reserve(Args.size());
  for (auto &&X : Args)
    ArgV.push_back(X->eval(C));
  return C->getFunction(Name)(C, std::move(ArgV));
}

enum TokenKind { TK_None = 0, TK_String = -1, TK_Number = -2, TK_END = -3 };

struct Token {
  int Kind;

  std::string Str{};

  bool operator==(std::string const &RHS) const noexcept {
    return Kind == TK_String && Str == RHS;
  }
  bool operator==(int RHS) const noexcept { return Kind == RHS; }

  double toNumber() const {
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

struct Calculation {
  using result_type = double;

  std::istringstream SS;

  Token CurrentToken = {0};

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

  std::unique_ptr<AST> parseExpr() { return parseBinExprRHS(parseUnary(), 0); }

  static const std::map<char, unsigned> Precedences;

  static bool isBinOp(const Token &Tok) noexcept {
    return Precedences.find(Tok.Kind) != Precedences.cend();
  }

  static int getPrecedence(const Token &Tok) {
    return Precedences.at(Tok.Kind);
  }

  static const std::set<char> RightCombinedOps;

  static bool isRightCombined(char C) noexcept {
    return RightCombinedOps.find(C) != RightCombinedOps.cend();
  }

  std::unique_ptr<AST> parseBinExprRHS(std::unique_ptr<AST> LHS, int Prec) {
    while (true) {
      const auto Tok = peekToken();
      if (!isBinOp(Tok) || getPrecedence(Tok) < Prec)
        return LHS;

      eatToken();
      auto RHS = parseUnary();
      const auto NextTok = peekToken();

      if (isBinOp(NextTok) && getPrecedence(NextTok) > Prec)
        RHS = parseBinExprRHS(std::move(RHS),
                              getPrecedence(Tok) +
                                  (isRightCombined(NextTok.Kind) ? 0 : 1));

      LHS = std::make_unique<BinExprAST>(std::move(LHS), std::move(RHS),
                                         Tok.Kind);
    }
  }

  std::unique_ptr<AST> parseUnary() {
    const auto Tok = peekToken();
    if (Tok == '-') {
      eatToken();
      return std::make_unique<UnaryExprAST>(parsePrimary(), '-');
    }
    return parsePrimary();
  }

  std::unique_ptr<AST> parsePrimary() {
    const auto Tok = peekToken();

    if (Tok == TK_Number) {
      eatToken();
      return std::make_unique<ConstExpr>(Tok.toNumber());
    }
    if (Tok == TK_String) {
      eatToken();
      auto StrTok = std::make_unique<IdentifierAST>(Tok.Str);
      if (peekToken() == '(') {
        eatToken();
        auto Args = parseArgList();
        assert(peekToken() == ')');
        eatToken();
        return std::make_unique<CallExprAST>(
            static_cast<IdentifierAST const &>(*StrTok).getName(),
            std::move(Args));
      }
      return StrTok;
    } else if (Tok == '(') {
      eatToken();
      auto Ret = parseExpr();
      assert(peekToken() == ')');
      eatToken();
      return Ret;
    } else
      throw CalculationError("Expected number, but got " +
                             Tok.getDescription());
  }

  std::vector<std::unique_ptr<AST>> parseArgList() {
    std::vector<std::unique_ptr<AST>> Ret;
    const auto Tok = peekToken();
    if (Tok == ')')
      return Ret;
    while (true) {
      Ret.push_back(parseExpr());
      if (peekToken() == ')')
        return Ret;
      if (peekToken() == ',')
        eatToken();
      else
        throw CalculationError("unknown token: " +
                               peekToken().getDescription());
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
};

const std::map<char, unsigned> Calculation::Precedences{
    {'=', 10}, {'+', 20}, {'-', 20}, {'*', 30}, {'/', 30}, {'^', 40},
};

const std::set<char> Calculation::RightCombinedOps{'^', '='};

double Calculator::calculate(std::string Expr) {
  Calculation C{std::istringstream(std::move(Expr))};
  auto Ast = C();
  return Ast->eval(this);
}

std::vector<std::string>
Calculator::getCompletionList(const std::string &Text) const {
  std::vector<std::string> Ret;

  for (const auto &Scope : VariableScopes) {
    for (const auto &Pair : Scope) {
      if (Pair.first.find(Text) == 0 && Pair.first.length() != Text.length())
        Ret.push_back(Pair.first);
    }
  }
  for (const auto &Scope : FunctionScopes) {
    for (const auto &Pair : Scope) {
      if (Pair.first.find(Text) == 0 && Pair.first.length() != Text.length())
        Ret.push_back(Pair.first);
    }
  }
  return Ret;
}

} // namespace calc
