#include "calculator.hpp"
#include <iostream>
#include <readline/history.h>
#include <readline/readline.h>

bool readExpr(std::string &Expr) {
  char *input = readline(">> ");
  if (!input)
    return false;
  add_history(input);
  try {
    Expr.assign(input);
  } catch (...) {
    free(input);
    throw;
  }
  free(input);
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