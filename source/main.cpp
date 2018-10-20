#include "interpreter.hpp"

#include <readline/history.h>
#include <readline/readline.h>

#include <cmath>
#include <iostream>

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

template <typename Callable>
lince::Function UnaryFunction(Callable &&Func)
{
    return { [Func = std::forward<Callable>(Func)](lince::Interpreter *,
                 std::vector<lince::Value> args) -> lince::Value {
                return { { Func(
                    std::any_cast<double>(args[0].Data)) } };
            },
        std::vector<std::type_index>{
            typeid(double), typeid(double) } };
}

template <typename Callable>
lince::Function BinaryFunction(Callable &&Func)
{
    return { [Func = std::forward<Callable>(Func)](lince::Interpreter *,
                 std::vector<lince::Value> args) -> lince::Value {
                return { { Func(
                    std::any_cast<double>(args[0].Data),
                    std::any_cast<double>(args[1].Data)) } };
            },
        std::vector<std::type_index>{
            typeid(double),
            typeid(double),
            typeid(double) } };
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
    Calc.setValue("pi", { 3.1415926535897 });
    Calc.setValue("e", { 2.718 });
    Calc.setFunction("sqrt",
        UnaryFunction(static_cast<double (*)(double)>(std::sqrt)));
    Calc.setFunction("exp",
        UnaryFunction(static_cast<double (*)(double)>(std::exp)));
    
    Calc.setFunction("operator-", BinaryFunction(std::minus<>()));
    Calc.setFunction("operator+", BinaryFunction(std::plus<>()));
    Calc.setFunction("operator*", BinaryFunction(std::multiplies<>()));
    Calc.setFunction("operator/", BinaryFunction(std::divides<>()));
    Calc.setFunction("operator^", BinaryFunction(static_cast<double (*)(double, double)>(std::pow)));

    std::string Expr;

    ::rl_attempted_completion_function = [](const char *Text, int, int) {
        rl_attempted_completion_over = 1;
        return rl_completion_matches(Text, CompletionGenerator);
    };

    while (readExpr(Expr)) {

        try {
            auto AST = Calc.parse(Expr);
            lince::Value V;
            Calc.eval(AST.get(), V);
            std::cout << V.stringof() << '\n';
        } catch (std::exception &E) {
            std::cout << "Error: " << E.what() << '\n';
        }
    }
}
