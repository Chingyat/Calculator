#pragma once
#include "ast.hpp"
#include "value.hpp"

#include <memory>
#include <string>
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
    const std::string &getName() const & { return Name; }
};

class UnaryExprAST : public AST {
    std::unique_ptr<AST> Operand;
    char Op;

public:
    UnaryExprAST(std::unique_ptr<AST> Operand, char Op) noexcept
        : Operand(std::move(Operand))
        , Op(Op)
    {
    }

    Value eval(Interpreter *C) final;
};

class BinExprAST : public AST {
    std::unique_ptr<AST> LHS, RHS;
    char Op;

public:
    BinExprAST(std::unique_ptr<AST> LHS, std::unique_ptr<AST> RHS,
        char Op) noexcept
        : LHS(std::move(LHS))
        , RHS(std::move(RHS))
        , Op(Op)
    {
    }

    Value eval(Interpreter *C) final;
};

class ConstExprAST : public AST {
    Value V;

public:
    explicit ConstExprAST(Value V) noexcept
        : V(V)
    {
    }

    Value eval(Interpreter *) noexcept final { return V; }
};

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

    std::vector<std::string> getParams() const;

    std::string const &getName() const &noexcept { return Name; }
};

class IfExprAST : public AST {
    std::unique_ptr<AST> Condition, Then, Else;

public:
    IfExprAST(std::unique_ptr<AST> Condition, std::unique_ptr<AST> Then, std::unique_ptr<AST> Else = nullptr)
        : Condition(std::move(Condition))
        , Then(std::move(Then))
        , Else(std::move(Else))
    {
    }

    Value eval(Interpreter *C) final;
};

}