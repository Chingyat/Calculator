#include "interpreter.hpp"
#include "stdlib.hpp"
#include "astfmt.hpp"

#include <readline/history.h>
#include <readline/readline.h>

#include <cmath>
#include <iostream>
#include <string_view>

struct free_str {
    void operator()(char *Str) noexcept { free(Str); }
};

bool readExpr(std::string &Expr)
{
    std::unique_ptr<char, free_str> Input(readline(">> "));
    if (!Input)
        return false;
    add_history(Input.get());
    Expr.assign(Input.get());
    return true;
}

lince::Interpreter Calc;

char *CompletionGenerator(const char *Text, int State)
{
    static std::set<std::string> Matches;
    static auto It = Matches.cend();

    if (State == 0) {
        Matches = Calc.getCompletionList(Text);
        It = Matches.cbegin();
    }

    if (It == Matches.cend()) {
        return nullptr;
    } else {
        return strdup(It++->c_str());
    }
}

int main()
{

    Calc.addModule(
        lince::StdLibModule());

    std::string Expr;

    ::rl_attempted_completion_function = [](const char *Text, int, int) {
        rl_attempted_completion_over = true;
        return rl_completion_matches(Text, CompletionGenerator);
    };

    rl_initialize();


    while (readExpr(Expr)) {
        try {
            auto AST = Calc.parse(Expr);
            if (!AST)
                continue;
            lince::Value V;
            std::cout << format(AST.get()) << '\n';
            Calc.eval(AST.get(), V);
            std::cout << V.Info() << '\n';
        } catch (std::exception &E) {
            std::cout << "Error: " << E.what() << '\n';
        }
    }
}
