#include "parser.hpp"
#include "astimpl.hpp"

#include <cassert>
#include <map>
#include <set>

namespace lince {

std::string Token::descriptionof() const
{
    switch (Kind) {
    case TK_Identifier:
        return Str;
    case TK_Number:
        return Str;
    case TK_If:
        return "<if>";
    case TK_Else:
        return "<else>";
    case TK_Then:
        return "<then>";
    default:
        if (Kind > 0)
            return std::string("`") + reinterpret_cast<const char(&)[]>(Kind) + "' (" + std::to_string(Kind) + ')';
        else
            return "<Error>";
    case TK_END:
        return "<END>";
    }
}

Token Parser::parseToken()
{
    int C;
    while (std::isspace((C = SS.get())))
        ;

    if (C == '"' || C == '\'') {
        const char Quote = C;
        std::string S;
        while (true) {
            C = SS.get();
            if (C == Quote && (S.empty() || S.back() != '\\'))
                return { TK_String, std::move(S) };
            S.push_back(C);
        }
    }

    if (std::isalpha(C) || C == '_') {
        std::string S;
        S.push_back(C);
        while (true) {
            C = SS.get();
            if (!std::isalnum(C) && C != '_')
                break;
            S.push_back(C);
        }
        SS.unget();

        if (auto It = Keywords.find(S);
            It != Keywords.cend())
            return { It->second, It->first };

        return { TK_Identifier, std::move(S) };
    }

    if (std::isdigit(C) || C == '.') {
        std::string S;
        do {
            if (C == '-' || C == '+') {
                if (S.empty() || (S.back() != 'e' && S.back() != 'E')) {
                    break;
                }
            }

            if (C == '.' && S.find('.') < S.length())
                break;
            S.push_back(C);
            C = SS.get();
        } while (std::isdigit(C) || C == '.' || C == 'e' || C == 'E' || C == '-' || C == '+');
        SS.unget();
        return { TK_Number, std::move(S) };
    }

    if (C == EOF || C == '\n') {
        SS.unget();
        return { TK_END };
    }

    if (C > 127 || C < 0) {
        throw ParseError("Non-ascii character: " + std::to_string(static_cast<unsigned>(C)));
    }

    return { C };
}

std::unique_ptr<AST> Parser::parseExpr()
{
    return parseBinOpRHS(parseUnary(), 0);
}

std::unique_ptr<AST> Parser::parseBinOpRHS(std::unique_ptr<AST> LHS, int Prec)
{
    while (true) {
        const auto Tok = peekToken();
        if (!isBinOp(Tok) || getPrecedence(Tok) < Prec)
            return LHS;

        eatToken();
        auto RHS = parseUnary();
        const auto NextTok = peekToken();

        if (isBinOp(NextTok) && getPrecedence(NextTok) > Prec)
            RHS = parseBinOpRHS(std::move(RHS),
                getPrecedence(Tok) + (isRightCombined(NextTok.Kind) ? 0 : 1));

        LHS = std::make_unique<BinExprAST>(std::move(LHS), std::move(RHS), Tok.Kind);
    }
}

std::unique_ptr<AST> Parser::parseUnary()
{
    const auto Tok = peekToken();
    if (isUnOp(Tok)) {
        eatToken();
        return std::make_unique<UnaryExprAST>(parsePrimary(), Tok.Kind);
    }
    return parsePrimary();
}

std::unique_ptr<AST> Parser::parsePrimary()
{
    const auto Tok = peekToken();

    if (peekToken() == TK_If)
        return parseIfExpr();
    if (peekToken() == TK_While)
        return parseWhileExpr();
    if (Tok == TK_String) {
        eatToken();
        return std::make_unique<ConstExprAST>(Value{ Tok.Str });
    }
    if (Tok == TK_Number) {
        eatToken();
        return std::make_unique<ConstExprAST>(Value{ Tok.numberof() });
    }
    if (Tok == TK_Identifier) {
        eatToken();
        auto Identifier = std::make_unique<IdentifierAST>(Tok.Str);
        if (peekToken() == '(') {
            eatToken();
            auto Args = parseArgList();
            if (peekToken().Kind != ')')
                throw ParseError("Expected `)', but got " + peekToken().descriptionof());
            eatToken();
            return std::make_unique<CallExprAST>(Identifier->getName(),
                std::move(Args));
        }
        return Identifier;
    }
    if (Tok == TK_True) {
        eatToken();
        return std::make_unique<ConstExprAST>(Value{ true });
    }
    if (Tok == TK_False) {
        eatToken();
        return std::make_unique<ConstExprAST>(Value{ false });
    }
    if (Tok == TK_Nil) {
        eatToken();
        return std::make_unique<ConstExprAST>(Value{});
    }
    if (Tok == '(') {
        eatToken();
        auto ParenExpr = parseExpr();
        if (peekToken().Kind != ')')
            throw ParseError("Expected `)', but got " + peekToken().descriptionof());
        eatToken();

        if (peekToken() == '(') {
            eatToken();
            auto Args = parseArgList();
            if (peekToken().Kind != ')')
                throw ParseError("Expected `)', but got " + peekToken().descriptionof());
            eatToken();
            return std::make_unique<LambdaCallExpr>(std::move(ParenExpr), std::move(Args));
        }

        return ParenExpr;
    } else
        throw ParseError("Expected primary expression, but got " + Tok.descriptionof());
}

std::vector<std::unique_ptr<AST>> Parser::parseArgList()
{
    std::vector<std::unique_ptr<AST>> Ret;
    const auto Tok = peekToken();
    if (Tok == ')')
        return Ret;
    while (true) {
        Ret.push_back(parseExpr());
        if (peekToken() == ')')
            return Ret;
        if (peekToken() == ',')
            eatToken();
        else
            throw ParseError(
                "unknown token: " + peekToken().descriptionof());
    }
}

std::unique_ptr<AST> Parser::parseIfExpr()
{
    assert(peekToken() == TK_If);
    eatToken();

    auto C = parseExpr();
    if (peekToken().Kind != TK_Then)
        throw ParseError("Expected `then', but got " + peekToken().descriptionof());
    eatToken();

    auto T = parseExpr();

    std::unique_ptr<AST> E;

    if (auto Tok = peekToken();
        Tok == TK_Else) {
        eatToken();
        E = parseExpr();
    }

    return std::make_unique<IfExprAST>(
        std::move(C), std::move(T), std::move(E));
}

std::unique_ptr<AST> Parser::parseWhileExpr()
{
    assert(peekToken() == TK_While);
    eatToken();
    auto C = parseExpr();
    if (peekToken().Kind != TK_Do)
        throw ParseError("Expected `do', but got " + peekToken().descriptionof());
    eatToken();
    auto T = parseExpr();
    return std::make_unique<WhileExprAST>(std::move(C), std::move(T));
}

}
