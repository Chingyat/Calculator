#include <utility>

#pragma once
#include "ast.hpp"
#include "value.hpp"

#include <fmt/format.h>
#include <memory>
#include <vector>

namespace lince {

class IdentifierAST : public AST {
    std::string Name;

public:
    explicit IdentifierAST(std::string Name)
        : Name(std::move(Name))
    {
    }

    Value eval(Interpreter *C) final;

    std::string dump() const final
    {
        return fmt::format("Identifier {{Name: \"{}\"}}", getName());
    }

    const std::string &getName() const & { return Name; }
};

class UnaryExprAST : public AST {
    std::unique_ptr<AST> Operand;
    int Op;

public:
    UnaryExprAST(std::unique_ptr<AST> Operand, int Op) noexcept
        : Operand(std::move(Operand))
        , Op(Op)
    {
    }

    Value eval(Interpreter *C) final;

    std::string dump() const final
    {
        return fmt::format("UnaryExpression {{Op: \"{}\",Operand: {}}}",
            reinterpret_cast<const char(&)[]>(Op), Operand->dump());
    }
};

class BinExprAST : public AST {
    std::unique_ptr<AST> LHS, RHS;
    int Op;

public:
    BinExprAST(std::unique_ptr<AST> LHS, std::unique_ptr<AST> RHS,
        int Op) noexcept
        : LHS(std::move(LHS))
        , RHS(std::move(RHS))
        , Op(Op)
    {
    }

    Value eval(Interpreter *C) final;

    std::string dump() const final
    {
        return fmt::format("BinaryExpression {{Op: \"{}\",LHS: {},RHS: {}}}",
            reinterpret_cast<const char(&)[]>(Op),
            LHS->dump(), RHS->dump());
    }
};

class ConstExprAST : public AST {
    Value V;

public:
    explicit ConstExprAST(Value V) noexcept
        : V(std::move(V))
    {
    }

    Value eval(Interpreter *) noexcept final { return V; }

    std::string dump() const final
    {
        return fmt::format("Constant {{Value: \"{} <{}>\"}}", V.stringof(), demangle(V.Data.type().name()));
    }
};


template <typename Sequence>
inline std::string dumpASTArray(Sequence &&Seq)
{
    std::string S = "[";
    for (auto &&X : Seq) {
        S += X->dump() + ',';
    }
    S.pop_back();
    S += ']';
    return S;
}

class CallExprAST : public AST {
    std::string Name;
    std::vector<std::unique_ptr<AST>> Args;

public:
    CallExprAST(std::string Name, std::vector<std::unique_ptr<AST>> Args)
        : Name(std::move(Name))
        , Args(std::move(Args))
    {
    }

    Value eval(Interpreter *C) final;

    std::string dump() const final
    {
        return fmt::format("CallExpression {{Name: \"{}\",Args: {}}}", Name, dumpASTArray(Args));
    }

    std::vector<std::string> getParams() const;

    std::string const &getName() const &noexcept { return Name; }
};

class LambdaCallExpr : public AST {
    std::unique_ptr<AST> Lambda;
    std::vector<std::unique_ptr<AST>> Args;

public:
    LambdaCallExpr(std::unique_ptr<AST> Lambda, std::vector<std::unique_ptr<AST>> Args)
        : Lambda(std::move(Lambda))
        , Args(std::move(Args))
    {
    }

    Value eval(Interpreter *C) final;

    std::string dump() const final
    {
        return fmt::format("LambdaCall {{Lambda: {},Args: {}}}", Lambda->dump(), dumpASTArray(Args));
    }
};

class IfExprAST : public AST {
    std::unique_ptr<AST> Condition, Then, Else;

public:
    IfExprAST(std::unique_ptr<AST> Condition, std::unique_ptr<AST> Then, std::unique_ptr<AST> Else)
        : Condition(std::move(Condition))
        , Then(std::move(Then))
        , Else(std::move(Else))
    {
    }

    Value eval(Interpreter *C) final;

    std::string dump() const final
    {
        return fmt::format(
            "IfExpression {{Condition: {},ThenClause: {},ElseClause: {}}}",
            Condition->dump(), Then->dump(), Else ? Else->dump() : "nil");
    }
};

class WhileExprAST : public AST {
    std::unique_ptr<AST> Condition, Body;
public:
    WhileExprAST(std::unique_ptr<AST> Condition, std::unique_ptr<AST> Body)
        : Condition(std::move(Condition))
        , Body(std::move(Body))
    {
    }

    Value eval(Interpreter *C) final
    {
        Value Ret;
        while (Condition->eval(C).booleanof()) {
            Ret = Body->eval(C);
        }
        return Ret;
    }

    std::string dump() const final
    {
        return fmt::format("WhileExpression {{Condition: {}, Body: {}}}",
                Condition->dump(), Body->dump());
    }
};

class TranslationUnitAST : public AST {
    std::vector<std::unique_ptr<AST>> ExprList;

public:
    explicit TranslationUnitAST(std::vector<std::unique_ptr<AST>> ExprList = {})
        : ExprList(std::move(ExprList))
    {
    }

    Value eval(Interpreter *C) final
    {
        std::for_each(ExprList.begin(), ExprList.end() - 1, [&](auto &&X) {
            X->eval(C);
        });

        return ExprList.back()->eval(C);
    }

    std::string dump() const final
    {
        return fmt::format("TranslationUnitAST {{ExpressionList: {}}}", dumpASTArray(ExprList));
    }
};

}