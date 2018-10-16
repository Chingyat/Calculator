#include "calculator.hpp"

namespace calc {
double IdentifierAST::eval(Calculator *C) { return C->getValue(getName()); }

double BinExprAST::eval(Calculator *C) {
  if (Op == '=') {
    double Ret;
    C->setValue(dynamic_cast<const IdentifierAST &>(*LHS).getName(),
                (Ret = RHS->eval(C)));
    return Ret;
  }

  auto L = LHS->eval(C);
  auto R = RHS->eval(C);

  switch (Op) {
  case '+':
    return L + R;
  case '-':
    return L - R;
  case '*':
    return L * R;
  case '/':
    return L / R;
  case '^':
    return std::pow(L, R);
  default:
    assert(0 && "Unknown operator");
  }
}

double CallExprAST::eval(Calculator *C) {
  std::vector<double> ArgV;
  ArgV.reserve(Args.size());
  for (auto &&X : Args)
    ArgV.push_back(X->eval(C));
  return C->getFunction(Name)(std::move(ArgV));
}

} // namespace calc