#pragma once
#include "value.hpp"

namespace lince {
class Interpreter;

class AST {
public:
    virtual ~AST() = default;
    virtual Value eval(Interpreter *) = 0;
    virtual std::string dump() const { return ""; }
};

} // namespace lince