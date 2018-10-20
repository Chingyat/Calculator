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
    lince::Module M;
    M.addValue("pi", { 3.1415926535897 });
    M.addValue("e", { 2.7182818284590 });
    M.addValue("phi", { 0.618033988 });
    M.addFunction("sqrt", UnaryFunction(static_cast<double (*)(double)>(std::sqrt)));
    M.addFunction("exp", UnaryFunction(static_cast<double (*)(double)>(std::exp)));
    M.addFunction("sin", UnaryFunction(static_cast<double (*)(double)>(std::sin)));
    M.addFunction("cos", UnaryFunction(static_cast<double (*)(double)>(std::cos)));
    M.addFunction("tan", UnaryFunction(static_cast<double (*)(double)>(std::tan)));
    M.addFunction("cbrt", UnaryFunction(static_cast<double (*)(double)>(std::cbrt)));
    M.addFunction("abs", UnaryFunction(static_cast<double (*)(double)>(std::abs)));
    M.addFunction("log", UnaryFunction(static_cast<double (*)(double)>(std::log)));
    M.addFunction("log10", UnaryFunction(static_cast<double (*)(double)>(std::log10)));

    M.addFunction("operator-", BinaryFunction(std::minus<>()));
    M.addFunction("operator+", BinaryFunction(std::plus<>()));
    M.addFunction("operator*", BinaryFunction(std::multiplies<>()));
    M.addFunction("operator/", BinaryFunction(std::divides<>()));
    M.addFunction("operator^", BinaryFunction(static_cast<double (*)(double, double)>(std::pow)));

    Calc.addModule(std::move(M));

    std::string Expr;

    ::rl_attempted_completion_function = [](const char *Text, int, int) {
        rl_attempted_completion_over = true;
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
