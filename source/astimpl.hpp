#pragma once
#include "ast.hpp"
#include "value.hpp"
#include "pseudortti.hpp"

#include <fmt/format.h>
#include <memory>
#include <vector>

namespace lince {

class IdentifierAST final : public AST {
    std::string Name;

public:
    explicit IdentifierAST(std::string Name)
        : AST(AK_Identifier)
        , Name(std::move(Name))
    {
    }

    Value eval(Interpreter *C) final;

    std::string dump() const final
    {
        return fmt::format("Identifier {{Name: \"{}\"}}", getName());
    }

    static bool classof(const AST *A) noexcept
    {
        return A->getKind() == AK_Identifier;
    }

    const std::string &getName() const & { return Name; }
};

class GenericCallExpr : public AST {
public:
    GenericCallExpr(AST_Kind Kind)
        : AST(Kind)
    {
    }

    Value eval(Interpreter *C) { return { {} }; }
    virtual std::string getFunctionName() const = 0;

    virtual std::vector<std::string> getParams() const = 0;

    static bool classof(const AST *A) noexcept
    {
        return A->getKind() > AK_GenericCallExpr && A->getKind() < AK_GenericCallExprEnd;
    }
};

class UnaryExprAST final : public GenericCallExpr {
    std::unique_ptr<AST> Operand;
    int Op;

public:
    UnaryExprAST(std::unique_ptr<AST> Operand, int Op) noexcept
        : GenericCallExpr(AK_UnaryExpr)
        , Operand(std::move(Operand))
        , Op(Op)
    {
    }

    Value eval(Interpreter *C) final;

    std::string dump() const final
    {
        return fmt::format("UnaryExpression {{Op: \"{}\",Operand: {}}}",
            reinterpret_cast<const char(&)[]>(Op), Operand->dump());
    }

    std::string getFunctionName() const final
    {
        return std::string("operator") + reinterpret_cast<const char(&)[]>(Op);
    }

    std::vector<std::string> getParams() const final
    {
        return { dyn_cast<IdentifierAST>(*Operand).getName() };
    }

    static bool classof(const AST *A) noexcept
    {
        return A->getKind() == AK_UnaryExpr;
    }
};

class BinExprAST final : public GenericCallExpr {
    std::unique_ptr<AST> LHS, RHS;
    int Op;

public:
    BinExprAST(std::unique_ptr<AST> LHS, std::unique_ptr<AST> RHS,
        int Op) noexcept
        : GenericCallExpr(AK_BinExpr)
        , LHS(std::move(LHS))
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

    std::string getFunctionName() const final
    {
        return std::string("operator") + reinterpret_cast<const char(&)[]>(Op);
    }

    std::vector<std::string> getParams() const final
    {
        return { dyn_cast<IdentifierAST>(*LHS).getName(), dyn_cast<IdentifierAST>(*RHS).getName() };
    }

    static bool classof(const AST *A) noexcept
    {
        return A->getKind() == AK_BinExpr;
    }
};

class ConstExprAST final : public AST {
    Value V;

public:
    explicit ConstExprAST(Value V) noexcept
        : AST(AK_ConstExpr)
        , V(std::move(V))
    {
    }

    Value eval(Interpreter *) noexcept final { return V; }

    std::string dump() const final
    {
        return fmt::format("Constant {{Value: \"{} <{}>\"}}", V.stringof(), TypeIDStr(V.TypeID()));
    }

    static bool classof(const AST *A) noexcept
    {
        return A->getKind() == AK_ConstExpr;
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

class CallExprAST final : public GenericCallExpr {
    std::string Name;
    std::vector<std::unique_ptr<AST>> Args;

public:
    CallExprAST(std::string Name, std::vector<std::unique_ptr<AST>> Args)
        : GenericCallExpr(AK_CallExpr)
        , Name(std::move(Name))
        , Args(std::move(Args))
    {
    }

    Value eval(Interpreter *C) final;

    std::string dump() const final
    {
        return fmt::format("CallExpression {{Name: \"{}\",Args: {}}}", Name, dumpASTArray(Args));
    }

    std::vector<std::string> getParams() const final;

    std::string getFunctionName() const noexcept final { return Name; }

    static bool classof(const AST *A) noexcept
    {
        return A->getKind() == AK_CallExpr;
    }
};

class LambdaCallExpr final : public AST {
    std::unique_ptr<AST> Lambda;
    std::vector<std::unique_ptr<AST>> Args;

public:
    LambdaCallExpr(std::unique_ptr<AST> Lambda, std::vector<std::unique_ptr<AST>> Args)
        : AST(AK_LambdaCallExpr)
        , Lambda(std::move(Lambda))
        , Args(std::move(Args))
    {
    }

    Value eval(Interpreter *C) final;

    std::string dump() const final
    {
        return fmt::format("LambdaCall {{Lambda: {},Args: {}}}", Lambda->dump(), dumpASTArray(Args));
    }

    static bool classof(const AST *A) noexcept
    {
        return A->getKind() == AK_LambdaCallExpr;
    }
};

class IfExprAST final : public AST {
    std::unique_ptr<AST> Condition, Then, Else;

public:
    IfExprAST(std::unique_ptr<AST> Condition, std::unique_ptr<AST> Then, std::unique_ptr<AST> Else)
        : AST(AK_IfExpr)
        , Condition(std::move(Condition))
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

    static bool classof(const AST *A) noexcept
    {
        return A->getKind() == AK_IfExpr;
    }
};

class WhileExprAST final : public AST {
    std::unique_ptr<AST> Condition, Body;

public:
    WhileExprAST(std::unique_ptr<AST> Condition, std::unique_ptr<AST> Body)
        : AST(AK_WhileExpr)
        , Condition(std::move(Condition))
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

    static bool classof(const AST *A) noexcept
    {
        return A->getKind() == AK_WhileExpr;
    }
};

class TranslationUnitAST final : public AST {
    std::vector<std::unique_ptr<AST>> ExprList;

public:
    explicit TranslationUnitAST(std::vector<std::unique_ptr<AST>> ExprList = {})
        : AST(AK_TranslationUnit)
        , ExprList(std::move(ExprList))
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

    static bool classof(const AST *A) noexcept
    {
        return A->getKind() == AK_TranslationUnit;
    }
};

}
