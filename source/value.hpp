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

    template <typename Type>
    bool isa() const noexcept
    {
        return std::any_cast<Type>(&Data);
    }

    bool booleanof() const noexcept
    {
        if (!Data.has_value())
            return false;
        if (isa<bool>())
            return std::any_cast<bool>(Data);
        if (isa<int>())
            return 0 != std::any_cast<int>(Data);
        return true;
    }

    std::string Info() const
    {
        return stringof();
    }

    std::string stringof() const
    {
        if (!Data.has_value())
            return "nil";
        if (isFunction())
            return "<Function>";
        if (isa<double>())
            return std::to_string(std::any_cast<double>(Data));
        if (isa<long double>())
            return std::to_string(std::any_cast<long double>(Data));
        if (isa<int>())
            return std::to_string(std::any_cast<int>(Data));
        if (isa<std::string>())
            return '\"' + std::any_cast<std::string>(Data) + '\"';
        if (isa<bool>())
            return std::any_cast<bool>(Data) ? "true" : "false";
        return "<Value>";
    }

    int TypeID() const;
};

struct Function {
    std::function<Value(Interpreter *, std::vector<Value>)> Data;
    std::vector<int> Type;

    template <typename Sequence>
    bool matchType(const Sequence &ArgType) const
    {
        return std::equal(std::cbegin(ArgType), std::cend(ArgType), Type.cbegin() + 1, Type.cend(),
            [](int LHS, int RHS) {
                return LHS == RHS;
            });
    }

    Value operator()(Interpreter *I, std::vector<Value> Args) const { return Data(
        I, std::move(Args)); }
};

inline bool Value::isFunction() const noexcept
{
    return isa<Function>();
}

template <typename T>
struct Signature;

template <typename T>
struct type_id_impl;

template <>
struct type_id_impl<int> : std::integral_constant<int, 'int'> {
};

template <>
struct type_id_impl<double> : std::integral_constant<int, 'dou'> {
};

template <>
struct type_id_impl<long double> : std::integral_constant<int, 'ld'> {
};

template <>
struct type_id_impl<Function> : std::integral_constant<int, 'Fun'> {
};

template <>
struct type_id_impl<std::string> : std::integral_constant<int, 'str'> {
};

template <>
struct type_id_impl<bool> : std::integral_constant<int, 'boo'> {
};

template <>
struct type_id_impl<void> : std::integral_constant<int, 'voi'> {
};

template <>
struct type_id_impl<Value> : std::integral_constant<int, 'Val'> {
};

template <typename T>
constexpr type_id_impl<std::decay_t<T>> type_id{};

inline int Value::TypeID() const
{
    if (!Data.has_value())
        return type_id<void>;
    if (isFunction())
        return type_id<Function>;
    if (isa<double>())
        return type_id<double>;
    if (isa<long double>())
        return type_id<long double>;
    if (isa<int>())
        return type_id<int>;
    if (isa<std::string>())
        return type_id<std::string>;
    if (isa<bool>())
        return type_id<bool>;
    return type_id<Value>;
}

template <typename R, typename... Args>
struct Signature<R(Args...)> {
    using Result = R;
    using Arguments = std::tuple<Args...>;

    // static std::vector<std::type_index> TypeIndices() { return {
    //     typeid(R), typeid(Args)...
    // }; }
    static std::vector<int> TypeIDs() { return {
        type_id<R>, type_id<Args>...
    }; }
};

template <typename Fn, typename... Args>
Value invokeForValue(Fn &&F, Args &&... A)
{
    if constexpr (std::is_void_v<std::invoke_result_t<Fn &&, Args &&...>>) {
        std::invoke(std::forward<Fn>(F), std::forward<Args>(A)...);
        return { {} };
    } else {
        return { std::invoke(std::forward<Fn>(F), std::forward<Args>(A)...) };
    }
}

inline std::string TypeIDStr(const int &TI)
{
    const auto &A = reinterpret_cast<const char(&)[4]>(TI);
    return {
        std::crbegin(A), std::crend(A)
    };
}

}
