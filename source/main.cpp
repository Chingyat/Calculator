#include "calculator.hpp"
#include <iostream>
#include <readline/history.h>
#include <readline/readline.h>

struct free_str {
  void operator()(char *Str) noexcept {
    free(Str);
  }
};

bool readExpr(std::string &Expr) {
  std::unique_ptr<char, free_str> Input(readline(">> "));
  if (!Input)
    return false;
  add_history(Input.get());
  Expr.assign(Input.get());
  return true;
}

int main() {
  calc::Calculator Calc;
  std::string Expr;

  while (readExpr(Expr)) {
    auto F = Calc.calculate<double>(Expr);

    try {
      std::cout << F.get() << '\n';
    } catch (std::exception &E) {
      std::cout << "Error: " << E.what() << '\n';
    }
  }
}
