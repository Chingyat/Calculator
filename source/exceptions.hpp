#pragma once

#include <exception>
#include <string>

namespace lince {

class ParseError : public std::exception {
    std::string Msg;

public:
    explicit ParseError(std::string Msg)
        : Msg(std::move(Msg))
    {
    }

    const char *what() const noexcept override { return Msg.c_str(); }
};

class EvalError : public std::exception {
    std::string Msg;

public:
    explicit EvalError(std::string Msg)
        : Msg(std::move(Msg))
    {
    }

    const char *what() const noexcept override { return Msg.c_str(); }
};

}