#pragma once
#include "interpreter.hpp"

namespace lince {

    inline bool isConvertible(Interpreter *C, const std::type_index &From, const std::type_index &To) noexcept
    {
        try {
            C->getFunction(ConstructorName(To.name()), std::vector<std::type_index>{ To, From });
            return true;
        } catch (EvalError &) {
            return false;
        }
    }

template <typename FwdIt1, typename FwdIt2>
inline bool areConvertible(Interpreter *C, FwdIt1 First, FwdIt1 Last, FwdIt2 OFirst, FwdIt2 OLast)
{
    if (std::distance(First, Last) != std::distance(OFirst, OLast))
        return false;
    while (First != Last) {
        if (!isConvertible(C, *First, *OFirst))
            return false;
    }
    return true;
}

template <typename Sequence>
Value Interpreter::callFunction(const std::string &Name, Sequence &&Args)
{
    auto Functions = findFunctions(Name);
    std::vector<std::type_index> ArgTypes;
    std::transform(std::cbegin(Args), std::cend(Args), std::back_inserter(ArgTypes),
        [](const Value &V) { return std::type_index(V.Data.type()); });

    const auto F = std::find_if(Functions.cbegin(), Functions.cend(),
        [&](const Function &Func) { return Func.matchType(ArgTypes); });

    if (F != Functions.cend())
        return invokeForValue(*F, this, std::forward<Sequence>(Args));

    const auto Compare = [&](const Function &X, const Function &Y) {
        unsigned L = areConvertible(this, ArgTypes.cbegin(), ArgTypes.cend(), X.Type.cbegin() + 1, X.Type.cend());
        unsigned R = areConvertible(this, ArgTypes.cbegin(), ArgTypes.cend(), Y.Type.cbegin() + 1, Y.Type.cend());
        return L < R;
    };

    std::sort(Functions.begin(), Functions.end(), Compare);

    const auto FirstMatch = std::find_if(Functions.cbegin(), Functions.cend(), [&](const Function &X) {
        return areConvertible(this, ArgTypes.cbegin(), ArgTypes.cend(), X.Type.cbegin() + 1, X.Type.cend());
    });

    const auto NCandidates = std::distance(FirstMatch, Functions.cend());
    if (NCandidates == 1) {
        auto Arg = std::begin(Args);
        auto Type = FirstMatch->get().Type.cbegin() + 1;
        while (Arg != Args.end()) {
            if (*Type != Arg->Data.type()) {
                std::vector<Value> ConversionArg;
                ConversionArg.emplace_back(std::move(*Arg));
                *Arg = callFunction(ConstructorName(Type->name()), std::move(ConversionArg));
            }
            ++Type;
            ++Arg;
        }
        return invokeForValue(*FirstMatch, this, std::forward<Sequence>(Args));
    }

    if (NCandidates > 1) {
        std::string Msg = "Ambiguous function call: \n";
        std::for_each(FirstMatch, Functions.cend(), [&](const Function &Func) {
            Msg += std::string("Candidate: ") + demangle(Func.Type.front().name()) + ' ' + Name + "(";
            std::for_each(Func.Type.begin() + 1, Func.Type.end(), [&](const std::type_index &TI) {
                Msg += std::string(" ") + demangle(TI.name()) + ',';
            });
            Msg.pop_back();
            Msg += " )\n";
        });
        throw EvalError(Msg);
    }

    // NCandidates == 0
    // Try dynamic functions
    const auto DynFunc = std::find_if(Functions.cbegin(), Functions.cend(), [](const Function &F) {
        return std::all_of(F.Type.cbegin() + 1, F.Type.cend(), [](const std::type_index &TI) {
            return TI == typeid(Value);
        });
    });

    if (DynFunc != Functions.cend())
        return invokeForValue(*DynFunc, this, std::forward<Sequence>(Args));

    // No match
    std::string Msg = "No such function: " + Name + ", arguments are: (";

    for (auto &&X : Args) {
        Msg += ' ' + X.Info() + ',';
    }

    Msg.pop_back();
    Msg += " )";

    throw EvalError(Msg);
}

template <typename Sequence>
Function DynamicFunction(Sequence &&ParamsV, std::shared_ptr<AST> Body)
{
    auto Params = std::make_shared<std::vector<std::string>>(std::forward<Sequence>(ParamsV));
    return { [Params, Body](Interpreter *C, std::vector<Value> Args) {
                const auto _ = C->createScope();
                const auto N = Params->size();
                for (size_t I = 0; I != N; ++I) {
                    C->addLocalValue(Params->at(I), Args[I]);
                }
                return Body->eval(C);
            },
        std::vector(Params->size() + 1, static_cast<std::type_index>(typeid(Value))) };
}

}