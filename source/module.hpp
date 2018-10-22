#pragma once
#include "value.hpp"

#include <map>
#include <string>

namespace lince {

inline std::string ConstructorName(int Name)
{
    return std::string("__") + reinterpret_cast<const char (&)[]>( Name );
}

template <typename Impl>
class ModuleBase {
    const Impl *self() const { return static_cast<Impl const *>(this); }
    Impl *self() { return static_cast<Impl *>(this); }

public:
    decltype(auto) getFunctionNS() &&
    {
        return std::move(self()->FunctionNS[0]);
    }

    decltype(auto) getValueNS() &&
    {
        return std::move(self()->ValueNS[0]);
    }

    const Function &addFunction(const std::string &Name, Function TheFunction)
    {
        auto It = self()->FunctionNS[0].emplace(Name, std::move(TheFunction));
        return It->second;
    }

    template <typename T, typename U>
    const Function &addConstructor()
    {
        Function F{
            [](Interpreter *, std::vector<Value> A) -> Value {
                return { T(
                    std::any_cast<U>(A[0].Data)) };
            },
            std::vector<int>{ type_id<T>, type_id<U> }
        };
        return addFunction(ConstructorName(type_id<T>), std::move(F));
    }

    const Value &addValue(const std::string &Name, Value TheValue)
    {
        return self()->ValueNS[0].emplace(Name, std::move(TheValue)).first->second;
    }
};

class Module : public ModuleBase<Module> {
    std::multimap<std::string, Function> FunctionNS[1];
    std::map<std::string, Value> ValueNS[1];
    friend class ModuleBase<Module>;

public:
};

template <typename Type, typename Callable = std::decay_t<Type>>
Function UnaryFunction(Callable Func)
{
    return { [Func = std::move(Func)](lince::Interpreter *, std::vector<lince::Value> args) {
                return invokeForValue(Func,
                    std::any_cast<std::tuple_element_t<0, typename lince::Signature<Type>::Arguments>>(
                        args[0].Data));
            },
        lince::Signature<Type>::TypeIDs() };
}

template <typename Type, typename Callable = std::decay_t<Type>>
Function BinaryFunction(Callable Func)
{
    return { [Func = std::move(Func)](lince::Interpreter *, std::vector<lince::Value> args) {
                return invokeForValue(Func,
                    std::any_cast<std::tuple_element_t<0, typename lince::Signature<Type>::Arguments>>(
                        args[0].Data),
                    std::any_cast<std::tuple_element_t<1, typename lince::Signature<Type>::Arguments>>(
                        args[1].Data));
            },
        lince::Signature<Type>::TypeIDs() };
}

}