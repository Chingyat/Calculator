#include "interpreter.hpp"
#include "ast.hpp"
#include "parser.hpp"
#include "demangle.hpp"

#include <typeindex>
#include <typeinfo>

namespace lince {

void Interpreter::eval(AST *MyAST, Value &Result)
{
    Result = MyAST->eval(this);
}

std::unique_ptr<AST> Interpreter::parse(const std::string &Expr) const
{
    Parser P{ std::istringstream{ Expr } };
    return P();
}

template <typename FwdIt1, typename FwdIt2>
static bool areConvertible(Interpreter *C, FwdIt1 First, FwdIt1 Last, FwdIt2 OFirst, FwdIt2 OLast)
{
    if (std::distance(First, Last) != std::distance(OFirst, OLast))
        return false;
    while (First != Last) {
        try {
            if (*First != *OFirst)
                C->getFunction(ConstructorName(OFirst->name()), std::vector{ *OFirst, *First });
            ++First;
            ++OFirst;
        } catch (EvalError &) {
            return false;
        }
    }
    return true;
}

Value Interpreter::callFunction(const std::string &Name, std::vector<Value> Args)
{
    auto Functions = findFunctions(Name);
    std::vector<std::type_index> ArgTypes;
    std::transform(Args.cbegin(), Args.cend(), std::back_inserter(ArgTypes),
        [](const Value &V) { return std::type_index(V.Data.type()); });

    const auto F = std::find_if(Functions.cbegin(), Functions.cend(),
        [&](const Function &Func) { return Func.matchType(ArgTypes); });

    if (F != Functions.cend())
        return std::invoke(*F, this, std::move(Args));

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
        auto Arg = Args.begin();
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
        return std::invoke(*FirstMatch, this, std::move(Args));
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
        throw EvalError(
            Msg);
    }

    throw EvalError("No such function");
}

std::set<std::string>
Interpreter::getCompletionList(const std::string &Text) const
{
    std::set<std::string> Ret;

    for (const auto &Scope : ValueNS) {
        for (const auto &Pair : Scope) {
            if (Pair.first.find(Text) == 0 && Pair.first.length() != Text.length())
                Ret.insert(Pair.first);
        }
    }
    for (const auto &Scope : FunctionNS) {
        for (const auto &Pair : Scope) {
            if (Pair.first.find(Text) == 0 && Pair.first.length() != Text.length())
                Ret.insert(Pair.first);
        }
    }
    return Ret;
}

Function DynamicFunction(std::vector<std::string> ParamsV, std::shared_ptr<AST> Body)
{
    auto Params = std::make_shared<std::vector<std::string>>(std::move(ParamsV));
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

} // namespace lince
