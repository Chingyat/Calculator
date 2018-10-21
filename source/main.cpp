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

template <typename Type, typename Callable>
lince::Function UnaryFunction(Callable &&Func)
{
    return { [Func = std::forward<Callable>(Func)](lince::Interpreter *, std::vector<lince::Value> args) {
                return lince::Value{ std::invoke(Func,
                    std::any_cast<std::tuple_element_t<0, typename lince::Signature<Type>::Arguments>>(args[0].Data)) };
            },
        lince::Signature<Type>::TypeIndices() };
}

template <typename Type, typename Callable>
lince::Function BinaryFunction(Callable &&Func)
{
    return { [Func = std::forward<Callable>(Func)](lince::Interpreter *, std::vector<lince::Value> args) {
                return lince::Value{ std::invoke(Func,
                    std::any_cast<std::tuple_element_t<0, typename lince::Signature<Type>::Arguments>>(args[0].Data),
                    std::any_cast<std::tuple_element_t<1, typename lince::Signature<Type>::Arguments>>(args[1].Data)) };
            },
        lince::Signature<Type>::TypeIndices() };
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

lince ::Module CalculatorModule()
{
    lince::Module M;
    M.addValue("pi", { 3.1415926535897 });
    M.addValue("e", { 2.7182818284590 });
    M.addValue("phi", { 0.618033988 });

    M.addFunction("sqrt", UnaryFunction<double(double)>(static_cast<double (*)(double)>(std::sqrt)));
    M.addFunction("exp", UnaryFunction<double(double)>(static_cast<double (*)(double)>(std::exp)));
    M.addFunction("sin", UnaryFunction<double(double)>(static_cast<double (*)(double)>(std::sin)));
    M.addFunction("cos", UnaryFunction<double(double)>(static_cast<double (*)(double)>(std::cos)));
    M.addFunction("tan", UnaryFunction<double(double)>(static_cast<double (*)(double)>(std::tan)));
    M.addFunction("cbrt", UnaryFunction<double(double)>(static_cast<double (*)(double)>(std::cbrt)));
    M.addFunction("abs", UnaryFunction<double(double)>(static_cast<double (*)(double)>(std::abs)));
    M.addFunction("log", UnaryFunction<double(double)>(static_cast<double (*)(double)>(std::log)));
    M.addFunction("log10", UnaryFunction<double(double)>(static_cast<double (*)(double)>(std::log10)));
    M.addFunction("operator-", UnaryFunction<double(double)>(std::negate<>()));

    M.addFunction("operator-", BinaryFunction<double(double, double)>(std::minus<>()));
    M.addFunction("operator+", BinaryFunction<double(double, double)>(std::plus<>()));
    M.addFunction("operator*", BinaryFunction<double(double, double)>(std::multiplies<>()));
    M.addFunction("operator/", BinaryFunction<double(double, double)>(std::divides<>()));
    M.addFunction("operator^", BinaryFunction<double(double, double)>(static_cast<double (*)(double, double)>(std::pow)));

    M.addConstructor<double, int>();
    M.addFunction("operator-", UnaryFunction<int(int)>(std::negate<>()));
    M.addFunction("operator-", BinaryFunction<int(int, int)>(std::minus<>()));
    M.addFunction("operator+", BinaryFunction<int(int, int)>(std::plus<>()));
    M.addFunction("operator*", BinaryFunction<int(int, int)>(std::multiplies<>()));
    M.addFunction("operator/", BinaryFunction<int(int, int)>(std::divides<>()));

    M.addFunction("operator+", BinaryFunction<std::string(std::string, std::string)>(std::plus<>()));
    M.addFunction("operator*", BinaryFunction<std::string(std::string, int)>([](const std::string &Str, unsigned N) {
        std::string S;
        while (N--)
            S += Str;
        return S;
    }));
    return M;
}

int main()
{

    Calc.addModule(
        CalculatorModule());

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
            std::cout << V.Info() << '\n';
        } catch (std::exception &E) {
            std::cout << "Error: " << E.what() << '\n';
        }
    }
}
