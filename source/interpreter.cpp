#include "interpreter.hpp"
#include "ast.hpp"
#include "demangle.hpp"
#include "parser.hpp"

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

const Value *Interpreter::findVariable(const std::string &Name) const noexcept
{
    for (auto Scope = ValueNS.crbegin(); Scope != ValueNS.crend();
         ++Scope) {
        const auto V = Scope->find(Name);
        if (V != Scope->cend())
            return &V->second;
    }
    return nullptr;
}

auto Interpreter::findFunctions(const std::string &Name) const noexcept
    -> std::vector<std::reference_wrapper<const Function>>
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

} // namespace lince
