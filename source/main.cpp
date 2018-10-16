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
calc::Calculator::Function UnaryFunction(Callable &&Func) {
  return [Func = std::forward<Callable>(Func)](calc::Calculator *C,
                                               std::vector<double> args) {
    return Func(args[0]);
  };
}
calc::Calculator Calc;

char *CompletionGenerator(const char *Text, int State) {
  static std::vector<std::string> Matches;
  static size_t MatchIndex = 0;

  if (State == 0) {
    MatchIndex = 0;

    Matches = Calc.getCompletionList(Text);
  }

  if (MatchIndex >= Matches.size()) {
    return nullptr;
  } else {
    return strdup(Matches[MatchIndex++].c_str());
  }
}

int main() {
  Calc.setValue("pi", 3.1415926535897);
  Calc.setValue("e", 2.718);
  Calc.setFunction("sqrt",
                   UnaryFunction(static_cast<double (&)(double)>(std::sqrt)));
  Calc.setFunction("exp",
                   UnaryFunction(static_cast<double (&)(double)>(std::exp)));

  std::string Expr;

  ::rl_attempted_completion_function = [](const char *Text, int, int) {
    rl_attempted_completion_over = 1;
    return rl_completion_matches(Text, CompletionGenerator);
  };

  while (readExpr(Expr)) {

    try {
      auto F = Calc.calculate(Expr);

      std::cout << F << '\n';
    } catch (std::exception &E) {
      std::cout << "Error: " << E.what() << '\n';
    }
  }
}
