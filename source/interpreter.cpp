#include "interpreter.hpp"
#include "ast.hpp"
#include "parser.hpp"

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

Value Interpreter::callFunction(const std::string &Name, std::vector<Value> Args)
{
    const auto Functions = findFunctions(Name);
    std::vector<std::type_index> ArgTypes;
    std::transform(Args.cbegin(), Args.cend(), std::back_inserter(ArgTypes), [](const Value &V) {
        return std::type_index(V.Data.type());
    });

    auto F = std::find_if(Functions.cbegin(), Functions.cend(), [&](const Function &Func) {
        return Func.matchType(ArgTypes);
    });

    if (F == Functions.cend())
        throw EvalError("No such function");

    return std::invoke(*F, this, std::move(Args));
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

Function DynamicFunction(std::vector<std::string> Params, std::unique_ptr<AST> Body)
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
