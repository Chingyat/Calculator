#pragma once
#include "value.hpp"

namespace lince {
class Interpreter;

class AST {
public:
    enum AST_Kind : int {
        AK_Identifier,

        AK_GenericCallExpr,
        AK_UnaryExpr,
        AK_BinExpr,
        AK_CallExpr,
        AK_GenericCallExprEnd,
        AK_ConstExpr,
        AK_LambdaCallExpr,
        AK_IfExpr,
        AK_WhileExpr,
        AK_TranslationUnit,
        AK_MAX,
    };

private:
    const AST_Kind Kind;

public:
    virtual ~AST() = default;
    virtual Value eval(Interpreter *) = 0;
    virtual std::string dump() const { return ""; }

    AST_Kind getKind() const noexcept { return Kind; }

protected:
    explicit AST(AST_Kind Kind)
        : Kind(Kind)
    {
    }
};

} // namespace lince