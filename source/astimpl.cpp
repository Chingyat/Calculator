#include "astimpl.hpp"
#include "interpreter.hpp"
#include "exceptions.hpp"

#include <cmath>
#include <vector>

namespace lince {

Value IdentifierAST::eval(Interpreter *C) { return C->getValue(getName()); }

std::vector<std::string> CallExprAST::getParams() const
{
    std::vector<std::string> Ret;
    Ret.reserve(Args.size());
    for (auto &&X : Args) {
        Ret.emplace_back(dynamic_cast<const IdentifierAST &>(*X).getName());
    }
    return Ret;
}

Value UnaryExprAST::eval(Interpreter *C)
{
    auto OperandV = Operand->eval(C);
    return C->callFunction(std::string("operator") + Op, std::vector{ std::move(OperandV) });
}

Value BinExprAST::eval(Interpreter *C)
{
    if (Op == '=') { // deal with assignments
        if (const auto Identifier = dynamic_cast<const IdentifierAST *>(LHS.get())) {
            const auto V = RHS->eval(C);
            C->setValue(Identifier->getName(), V);
            return V;
        }
        if (const auto Func = dynamic_cast<const CallExprAST *>(LHS.get())) {
            auto F = DynamicFunction(Func->getParams(), std::move(RHS));
            return { C->setFunction(Func->getName(), std::move(F)) };
        }

        throw ParseError("Syntax Error ");
    }

    auto L = LHS->eval(C);
    auto R = RHS->eval(C);

    //C->getFunction(std::string("operator") + Op)(C, std::vector<lince::Value>{ std::move(L), std::move(R) });
    return C->callFunction(std::string("operator") + Op, std::vector{ std::move(L), std::move(R) });
}

Value CallExprAST::eval(Interpreter *C)
{
    std::vector<Value> ArgV;
    ArgV.reserve(Args.size());
    for (auto &&X : Args)
        ArgV.push_back(X->eval(C));
    return C->callFunction(Name, std::move(ArgV));
}
} // namespace lince
