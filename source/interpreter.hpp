#pragma once
#include "ast.hpp"
#include "exceptions.hpp"
#include "value.hpp"

#include <cassert>
#include <cmath>
#include <functional>
#include <map>
#include <memory>
#include <set>
#include <sstream>
#include <vector>

namespace lince {

class AST;

template <typename Impl>
class ModuleBase {
    const Impl *self() const { return static_cast<Impl const *>(this); }
    Impl *self() { return static_cast<Impl *>(this); }

public:
    std::multimap<std::string, Function> getFunctionNS() &&
    {
        return std::move(self()->FunctionNS[0]);
    }

    std::map<std::string, Value> getValueNS() &&
    {
        return std::move(self()->ValueNS[0]);
    }

    const Function &addFunction(const std::string &Name, Function TheFunction)
    {
        auto It = self()->FunctionNS[0].emplace(Name, std::move(TheFunction));
        return It->second;
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

class Interpreter : public ModuleBase<Interpreter> {
    friend class ModuleBase<Interpreter>;

public:
    struct ScopeGuard {
        Interpreter *Calc;

        explicit ScopeGuard(Interpreter *C) noexcept
            : Calc(C)
        {
        }

        ~ScopeGuard()
        {
            Calc->ValueNS.pop_back();
            Calc->FunctionNS.pop_back();
        }
    };

    ScopeGuard createScope()
    {
        ValueNS.emplace_back();
        FunctionNS.emplace_back();
        return ScopeGuard(this);
    }

    std::unique_ptr<AST> parse(const std::string &Expr) const;

    void eval(AST *MyAST, Value &Result);

    Value getValue(const std::string &Name) const
    {
        if (auto V = findVariable(Name))
            return *V;
        throw EvalError("No such variable: " + Name);
    }

    const Value &setValue(const std::string &Name, Value V)
    {
        if (auto Var = findVariable(Name))
            return const_cast<Value &>(*Var) = std::move(V);

        return ValueNS.back()[Name] = std::move(V);
    }

    const Value &addLocalValue(const std::string &Name, Value V)
    {
        return ValueNS.back()[Name] = std::move(V);
    }

    Function const &getFunction(const std::string &Name, std::vector<std::type_index> const &Type) const &
    {
        const auto Functions = findFunctions(Name);
        const auto It = std::find_if(Functions.cbegin(), Functions.cend(), [&](Function const &F) {
            return F.Type == Type;
        });

        if (It == Functions.cend())
            throw EvalError("No such function");
        return *It;
    }

    const Function &addLocalFunction(const std::string &Name, Function Func)
    {
        auto It = FunctionNS.back().emplace(Name, std::move(Func));
        return It->second;
    }

    Value callFunction(const std::string &Name, std::vector<Value> Args);

    std::set<std::string> getCompletionList(const std::string &Text) const;

    template <typename ModuleImpl>
    void addModule(ModuleBase<ModuleImpl> &&M)
    {
        FunctionNS.emplace_back(std::move(M).getFunctionNS());
        ValueNS.emplace_back(std::move(M).getValueNS());
    }

private:
    const Value *findVariable(const std::string &Name) const noexcept
    {
        for (auto Scope = ValueNS.crbegin(); Scope != ValueNS.crend();
             ++Scope) {
            const auto V = Scope->find(Name);
            if (V != Scope->cend())
                return &V->second;
        }
        return nullptr;
    }

    auto findFunctions(const std::string &Name) const noexcept -> std::vector<std::reference_wrapper<const Function>>
    {
        std::vector<std::reference_wrapper<const Function>> Ret;
        std::for_each(FunctionNS.crbegin(), FunctionNS.crend(), [&](const auto &Scope) {
            auto [Begin, End] = Scope.equal_range(Name);
            std::for_each(Begin, End, [&](const auto &Pair) {
                Ret.emplace_back(Pair.second);
            });
        });
        return Ret;
    }

    std::vector<std::multimap<std::string, Function>> FunctionNS;
    std::vector<std::map<std::string, Value>> ValueNS;

    ScopeGuard SG = createScope();
};

inline Function DynamicFunction(std::vector<std::string> Params, std::unique_ptr<AST> Body)
{
    std::vector<std::type_index> Type(Params.size() + 1, typeid(Value));

    auto Data = [Params = std::make_shared<std::vector<std::string>>(std::move(Params)), Body = std::shared_ptr(std::move(Body))](
                    Interpreter *C, std::vector<Value> Args) {
        const auto _ = C->createScope();
        const auto N = Params->size();
        for (size_t I = 0; I != N; ++I) {
            C->addLocalValue(Params->at(I), Args[I]);
        }
        return Body->eval(C);
    };

    return { Data, Type };
}

} // namespace lince
