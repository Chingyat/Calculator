#pragma once
#include "demangle.hpp"

#include <any>
#include <functional>
#include <memory>
#include <string>
#include <typeindex>
#include <vector>

namespace lince {

class AST;
class Interpreter;

struct Value {
    std::any Data;

    bool isFunction() const noexcept;

    bool booleanof() const
    {
        if (!Data.has_value())
            return false;
        if (Data.type() == typeid(bool))
            return std::any_cast<bool>(Data);
        return true;
    }

    std::string Info() const { return {
        stringof() + " : " + demangle(Data.type().name())
    }; }

    std::string stringof() const
    {
        if (!Data.has_value())
            return "nil";
        if (isFunction())
            return "<Function>";
        if (Data.type() == typeid(double))
            return std::to_string(std::any_cast<double>(Data));
        if (Data.type() == typeid(long double))
            return std::to_string(std::any_cast<long double>(Data));
        if (Data.type() == typeid(int))
            return std::to_string(std::any_cast<int>(Data));
        if (Data.type() == typeid(std::string))
            return '\"' + std::any_cast<std::string>(Data) + '\"';
        if (Data.type() == typeid(bool))
            return std::any_cast<bool>(Data) ? "true" : "false";
        return "<Value>";
    }
};

struct Function {
    std::function<Value(Interpreter *, std::vector<Value>)> Data;
    std::vector<std::type_index> Type;

    template <typename Sequence>
    bool matchType(const Sequence &ArgType) const
    { // TODO: consider covariance and contravariance
        return std::equal(std::cbegin(ArgType), std::cend(ArgType), Type.cbegin() + 1, Type.cend(),
            [](const std::type_index &LHS, const std::type_index &RHS) {
                return LHS == typeid(Value) || RHS == typeid(Value) || LHS == RHS;
            });
    }

    Value operator()(Interpreter *I, std::vector<Value> Args) const { return Data(
        I, std::move(Args)); }
};

inline bool Value::isFunction() const noexcept
{
    return typeid(Function) == Data.type();
}

template <typename T>
struct Signature;

template <typename R, typename... Args>
struct Signature<R(Args...)> {
    using Result = R;
    using Arguments = std::tuple<Args...>;

    static std::vector<std::type_index> TypeIndices() { return {
        typeid(R), typeid(Args)...
    }; }
};

template <typename Fn, typename... Args>
Value invokeFunction(Fn &&F, Args &&... A)
{
    if constexpr (std::is_void_v<std::invoke_result_t<Fn &&, Args &&...>>) {
        std::invoke(std::forward<Fn>(F), std::forward<Args>(A)...);
        return { {} };
    } else {
        return { std::invoke(std::forward<Fn>(F), std::forward<Args>(A)...) };
    }
}

}
