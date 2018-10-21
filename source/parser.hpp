#pragma once
#include "ast.hpp"
#include "exceptions.hpp"
#include "value.hpp"

#include <map>
#include <memory>
#include <set>
#include <sstream>
#include <string>
#include <vector>

namespace lince {

enum TokenKind { TK_None = 0,
    TK_Identifier = -1,
    TK_Number = -2,
    TK_END = -3,
    TK_If = -4,
    TK_Then = -5,
    TK_Else = -6,
    TK_True = -7,
    TK_False = -8,
    TK_Nil = -9,
    TK_String = -10,
    TK_While = -11,
    TK_Do = -12 };

struct Token {
    int Kind;

    std::string Str{};

    bool operator==(std::string const &RHS) const noexcept
    {
        return Kind == TK_Identifier && Str == RHS;
    }
    bool operator==(int RHS) const noexcept { return Kind == RHS; }

    Value numberof() const
    {
        return Str.find('.') != std::string::npos ? Value{ std::stod(Str) }
                                                  : Value{ std::stoi(Str) };
    }

    std::string descriptionof() const;
};

struct Parser {
    using result_type = Value;

    std::istringstream SS;

    Token CurrentToken = { 0 };

    Token parseToken();

    Token peekToken()
    {
        if (CurrentToken == 0)
            CurrentToken = parseToken();
        return CurrentToken;
    }

    void eatToken() { CurrentToken = parseToken(); }

    std::unique_ptr<AST> parseExpr();

    const std::map<int, unsigned> Precedences{
        { '=', 10 },
        { '+', 20 },
        { '-', 20 },
        { '*', 30 },
        { '/', 30 },
        { '^', 40 },
    };

    const std::map<std::string, int> Keywords{
        { "if", TK_If },
        { "then", TK_Then },
        { "else", TK_Else },
        { "true", TK_True },
        { "false", TK_False },
        { "nil", TK_Nil },
        { "while", TK_While },
        { "do", TK_Do },
    };

    const std::set<int> UnaryOperators{'-', '!', '~'};

    const std::set<int> RightCombinedOps{ '^', '=' };

    bool isBinOp(const Token &Tok) noexcept
    {
        return Precedences.find(Tok.Kind) != Precedences.cend();
    }

    bool isUnOp(const Token &Tok) noexcept
    {
        return UnaryOperators.find(Tok.Kind) != UnaryOperators.cend();
    }

    int getPrecedence(const Token &Tok)
    {
        return Precedences.at(Tok.Kind);
    }

    bool isRightCombined(int C) noexcept
    {
        return RightCombinedOps.find(C) != RightCombinedOps.cend();
    }

    std::unique_ptr<AST> parseBinOpRHS(std::unique_ptr<AST> LHS, int Prec);

    std::unique_ptr<AST> parseUnary();

    std::unique_ptr<AST> parsePrimary();

    std::vector<std::unique_ptr<AST>> parseArgList();

    std::unique_ptr<AST> parseIfExpr();

    std::unique_ptr<AST> parseWhileExpr();

    std::unique_ptr<AST> operator()()
    {
        if (peekToken() == TK_END)
            return nullptr;
        auto V = parseExpr();
        if (peekToken() == TK_END) {
            return V;
        }
        throw ParseError("Unexpected trailing tokens " + peekToken().descriptionof());
    }
};

}