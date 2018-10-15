#include "calculator.hpp"
#include <iostream>
#include <readline/history.h>
#include <readline/readline.h>

struct free_str {
  void operator()(char *str) noexcept {
    free(str);
  }
};

bool readExpr(std::string &Expr) {
  std::unique_ptr<char, free_str> input(readline(">> "));
  if (!input)
    return false;
  add_history(input.get());
  Expr.assign(input.get());
  return true;
}

int main() {
  calc::Calculator Calc;
  std::string expr;

  while (readExpr(expr)) {
    auto F = Calc.calculate<double>(expr);

    try {
      std::cout << F.get() << '\n';
    } catch (std::exception &CE) {
      std::cout << "Error: " << CE.what() << '\n';
    }
  }
}
