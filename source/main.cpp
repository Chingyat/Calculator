#include "calculator.hpp"
#include <iostream>
#include <readline/history.h>
#include <readline/readline.h>

struct free_str {
  void operator()(char *Str) noexcept { free(Str); }
};

bool readExpr(std::string &Expr) {
  std::unique_ptr<char, free_str> Input(readline(">> "));
  if (!Input)
    return false;
  add_history(Input.get());
  Expr.assign(Input.get());
  return true;
}

template <typename Callable>
std::function<double(std::vector<double>)> UnaryFunction(Callable &&Func) {
  return [Func = std::forward<Callable>(Func)](std::vector<double> args) {
    return Func(args[0]);
  };
}

int main() {
  calc::Calculator Calc;
  Calc.setValue("pi", 3.1415926535897);
  Calc.setValue("e", 2.718);
  Calc.setFunction("sqrt",
                   UnaryFunction(static_cast<double (&)(double)>(std::sqrt)));
  Calc.setFunction("exp",
                   UnaryFunction(static_cast<double (&)(double)>(std::exp)));

  std::string Expr;

  while (readExpr(Expr)) {

    try {
      auto F = Calc.calculate(Expr);

      std::cout << F << '\n';
    } catch (std::exception &E) {
      std::cout << "Error: " << E.what() << '\n';
    }
  }
}
