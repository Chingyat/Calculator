#include "parser.hpp"
#include "astimpl.hpp"

#include <map>
#include <set>

namespace lince {

const std::map<char, unsigned> Parser::Precedences{
    { '=', 10 },
    { '+', 20 },
    { '-', 20 },
    { '*', 30 },
    { '/', 30 },
    { '^', 40 },
};

const std::set<char> Parser::RightCombinedOps{ '^', '=' };

std::string Token::getDescription() const
{
    if (Kind == TK_String)
        return Str;
    if (Kind == TK_Number)
        return Str;
    if (Kind > 0)
        return std::string("`") + static_cast<char>(Kind) + "' (" + std::to_string(Kind) + ')';
    if (Kind == TK_END)
        return "<END>";
    return "<Err>";
}

Token Parser::parseToken()
{
    int C;
    while (std::isspace((C = SS.get())))
        ;

    if (std::isalpha(C)) {
        std::string S;
        S.push_back(C);
        while (std::isalnum((C = SS.get()))) {
            S.push_back(C);
        }
        SS.unget();
        return { TK_String, std::move(S) };
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

    return { C };
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

        LHS = std::make_unique<BinExprAST>(std::move(LHS), std::move(RHS),
            Tok.Kind);
    }
}

std::unique_ptr<AST> Parser::parseUnary()
{
    const auto Tok = peekToken();
    if (Tok == '-') {
        eatToken();
        return std::make_unique<UnaryExprAST>(parsePrimary(), '-');
    }
    return parsePrimary();
}

std::unique_ptr<AST> Parser::parsePrimary()
{
    const auto Tok = peekToken();

    if (Tok == TK_Number) {
        eatToken();
        return std::make_unique<ConstExpr>(Value{ Tok.toNumber() });
    }
    if (Tok == TK_String) {
        eatToken();
        auto Identifier = std::make_unique<IdentifierAST>(Tok.Str);
        if (peekToken() == '(') {
            eatToken();
            auto Args = parseArgList();
            assert(peekToken() == ')');
            eatToken();
            return std::make_unique<CallExprAST>(Identifier->getName(),
                std::move(Args));
        }
        return Identifier;
    } else if (Tok == '(') {
        eatToken();
        auto Ret = parseExpr();
        assert(peekToken() == ')');
        eatToken();
        return Ret;
    } else
        throw ParseError("Expected primary expression, but got " + Tok.getDescription());
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
            throw ParseError("unknown token: " + peekToken().getDescription());
    }
}
}