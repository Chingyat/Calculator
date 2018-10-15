#include "calculator.hpp"

int main() {
  calc::Calculation<int> c{std::istringstream("1+1")};

  auto F1 = c.parseExpr();
  assert(F1 == 2);

  calc::Calculator Calc;

  auto F2 = Calc.calculate<double>("5 * 2 + 2^3 * 3^2").get();

  assert(std::fabs(F2 - 82) < 0.00001);
}
