#pragma once
#include <cassert>
#include <cmath>
#include <future>
#include <optional>
#include <sstream>
#include <string_view>

namespace calc {

enum TokenKind { TK_None = 0, TK_String = -1, TK_Number = -2, TK_END = -3 };

struct Token {
  int Kind;

  std::string Str{};

  bool operator==(std::string_view RHS) const noexcept {
    return Kind == TK_String && Str == RHS;
  }
  bool operator==(int RHS) const noexcept { return Kind == RHS; }

  template <typename T> T toNumber() const noexcept {
    std::istringstream SS(Str);
    T N;
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

template <typename T> struct Calculation {
  using result_type = T;

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

  T parseExpr() {
    auto V = parseTerm();

    while (true) {
      if (auto Op = peekToken(); Op == '+' || Op == '-') {
        eatToken();
        auto V2 = parseTerm();
        if (Op == '+')
          V = V + V2;
        else
          V = V - V2;
        continue;
      }
      return V;
    }
  }

  T parseTerm() {
    auto V = parsePower();
    while (true) {
      if (auto Op = peekToken(); Op == '*' || Op == '/') {
        eatToken();
        auto V2 = parsePower();
        if (Op == '*')
          V = V * V2;
        else
          V = V / V2;
        continue;
      }
      return V;
    }
  }

  T parsePower() {
    auto V = parseParen();
    if (auto Op = peekToken(); Op == '^') {
      eatToken();
      return std::pow(V, parsePower());
    }
    return V;
  }

  T parseParen() {
    if (auto Tok = peekToken(); Tok == '(') {
	  eatToken();
      auto V = parseExpr();
      Tok = peekToken();
      assert(Tok == ')');
      eatToken();
      return V;
    }
    return parseLiteral();
  }

  T parseLiteral() {
    auto F = [&]() -> T (*)(T) noexcept {
      if (auto Tok = peekToken(); Tok == '+') {
        eatToken();
        return [](T X) noexcept { return X; };
      } else if (Tok == '-') {
        eatToken();
        return [](T X) noexcept { return -X; };
      } else {
        return [](T X) noexcept { return X; };
      }
    }();

    auto Tok = peekToken();
    eatToken();

    T V;

    if (Tok == TK_Number)
      V = Tok.template toNumber<T>();
    else
      throw CalculationError("Expected number, but got " +
                             Tok.getDescription());

    return F(V);
  }

  T operator()() {
    auto V = parseExpr();
    if (peekToken() == TK_END) {
      return V;
    }
    throw CalculationError("Unexpected trailing tokens " +
                               peekToken().getDescription(),
                           V);
  }

  class CalculationError : public std::exception {
    std::optional<T> Result;
    std::string Msg;

  public:
    CalculationError(std::string Msg, std::optional<T> Result = std::nullopt)
        : Result(std::move(Result)), Msg(std::move(Msg)) {}

    const char *what() const noexcept { return Msg.c_str(); }

    const std::optional<T> &getResult() const & noexcept { return Result; }
  };
};

class Calculator {
public:
  template <typename T> std::future<T> calculate(std::string Expr) noexcept {
    return std::async(Calculation<T>{std::istringstream(std::move(Expr))});
  }
};
} // namespace calc
