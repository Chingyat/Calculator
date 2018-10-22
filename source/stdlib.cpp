#include "stdlib.hpp"
#include "interpreter.hpp"

namespace lince {

StdLibModule::StdLibModule()
{
    addValue("pi", { 3.1415926535897 });
    addValue("e", { 2.7182818284590 });
    addValue("phi", { 0.618033988 });
    addFunction("sqrt", UnaryFunction<double(double)>(std::sqrt));
    addFunction("exp", UnaryFunction<double(double)>((std::exp)));
    addFunction("sin", UnaryFunction<double(double)>(std::sin));
    addFunction("cos", UnaryFunction<double(double)>(std::cos));
    addFunction("tan", UnaryFunction<double(double)>(std::tan));
    addFunction("cbrt", UnaryFunction<double(double)>(std::cbrt));
    addFunction("abs", UnaryFunction<double(double)>(std::abs));
    addFunction("log", UnaryFunction<double(double)>(std::log));
    addFunction("log10", UnaryFunction<double(double)>(std::log10));
    addFunction("operator-", UnaryFunction<double(double)>(std::negate<>()));
    addFunction("operator-", BinaryFunction<double(double, double)>(std::minus<>()));
    addFunction("operator+", BinaryFunction<double(double, double)>(std::plus<>()));
    addFunction("operator*", BinaryFunction<double(double, double)>(std::multiplies<>()));
    addFunction("operator/", BinaryFunction<double(double, double)>(std::divides<>()));
    addFunction("operator^", BinaryFunction<double(double, double)>(std::pow));
    addConstructor<double, int>();
    addFunction("operator-", UnaryFunction<int(int)>(std::negate<>()));
    addFunction("operator-", BinaryFunction<int(int, int)>(std::minus<>()));
    addFunction("operator+", BinaryFunction<int(int, int)>(std::plus<>()));
    addFunction("operator*", BinaryFunction<int(int, int)>(std::multiplies<>()));
    addFunction("operator/", BinaryFunction<int(int, int)>(std::divides<>()));
    addFunction("operator+", BinaryFunction<std::string(std::string, std::string)>(std::plus<>()));
    addFunction("operator*", BinaryFunction<std::string(std::string, int)>([](const std::string &Str, unsigned N) {
        std::string S;
        while (N--)
            S += Str;
        return S;
    }));
    addFunction("operator;", { [](const auto &, std::vector<Value> A) {
                                  return A[1];
                              },
                                 std::vector<std::type_index>(3u, typeid(Value)) });

    addFunction("int", UnaryFunction<int(double)>([](double x) { return int(x); }));
    addFunction("exit", UnaryFunction<void(int)>(std::exit));
    addFunction("string", UnaryFunction<std::string(int)>(std::to_string));
    addFunction("string", UnaryFunction<std::string(double)>(std::to_string));

    addFunction("write_line", UnaryFunction<void(std::string const &)>([](const auto &S) {
        return std::puts(S.c_str());
    }));
}
} // lince
