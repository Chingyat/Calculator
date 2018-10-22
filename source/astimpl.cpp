#include "astimpl.hpp"
#include "exceptions.hpp"
#include "interpreter.hpp"
#include "pseudortti.hpp"

namespace lince {

Value IdentifierAST::eval(Interpreter *C) { return C->getValue(getName()); }

std::vector<std::string> CallExprAST::getParams() const
{
    std::vector<std::string> Ret;
    Ret.reserve(Args.size());
    for (auto &&X : Args) {
        Ret.emplace_back(dyn_cast<IdentifierAST>(*X).getName());
    }
    return Ret;
}

Value UnaryExprAST::eval(Interpreter *C)
{
    std::vector<Value> Arg;
    Arg.emplace_back(Operand->eval(C));
    return C->callFunction(std::string("operator") + reinterpret_cast<char(&)[]>(Op), std::move(Arg));
}

Value BinExprAST::eval(Interpreter *C)
{
    if (Op == '=') { // deal with assignments
        if (const auto Identifier = dyn_cast<IdentifierAST>(LHS.get())) {
            const auto V = RHS->eval(C);
            C->setValue(Identifier->getName(), V);
            return V;
        }
        if (const auto Func = dyn_cast<GenericCallExpr>(LHS.get())) {
            auto F = DynamicFunction(Func->getParams(), std::move(RHS));
            return { C->addLocalFunction(Func->getFunctionName(), std::move(F)) };
        }

        throw ParseError("Syntax Error ");
    }

    std::vector<Value> Operands;
    Operands.emplace_back(LHS->eval(C));
    Operands.emplace_back(RHS->eval(C));

    return C->callFunction(std::string("operator") + reinterpret_cast<char(&)[]>(Op), std::move(Operands));
}

Value CallExprAST::eval(Interpreter *C)
{
    std::vector<Value> ArgV;
    ArgV.reserve(Args.size());
    for (auto &&X : Args)
        ArgV.push_back(X->eval(C));
    return C->callFunction(Name, std::move(ArgV));
}

Value LambdaCallExpr::eval(Interpreter *C)
{
    auto L = Lambda->eval(C);
    std::vector<Value> ArgV;
    ArgV.reserve(Args.size());
    std::transform(Args.cbegin(), Args.cend(), std::back_inserter(ArgV),
        [&](const auto &X) { return X->eval(C); });
    return invokeForValue(std::any_cast<Function>(L.Data), C, std::move(ArgV));
}

Value IfExprAST::eval(Interpreter *C)
{
    if (Condition->eval(C).booleanof()) {
        return Then->eval(C);
    } else if (Else) {
        return Else->eval(C);
    }
    return {};
}
} // namespace lince
