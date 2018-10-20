#pragma once
#include "ast.hpp"
#include "exceptions.hpp"
#include "value.hpp"

#include <cassert>
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
    TK_String = -10 };

struct Token {
    int Kind;

    std::string Str{};

    bool operator==(std::string const &RHS) const noexcept
    {
        return Kind == TK_Identifier && Str == RHS;
    }
    bool operator==(int RHS) const noexcept { return Kind == RHS; }

    Value numberof() const { return { std::stod(Str) }; }

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

    static const std::map<char, unsigned> Precedences;

    static bool isBinOp(const Token &Tok) noexcept
    {
        return Precedences.find(Tok.Kind) != Precedences.cend();
    }

    static int getPrecedence(const Token &Tok)
    {
        return Precedences.at(Tok.Kind);
    }

    static const std::set<char> RightCombinedOps;

    static bool isRightCombined(char C) noexcept
    {
        return RightCombinedOps.find(C) != RightCombinedOps.cend();
    }

    std::unique_ptr<AST> parseBinOpRHS(std::unique_ptr<AST> LHS, int Prec);

    std::unique_ptr<AST> parseUnary();

    std::unique_ptr<AST> parsePrimary();

    std::vector<std::unique_ptr<AST>> parseArgList();

    std::unique_ptr<AST> parseIfExpr();

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