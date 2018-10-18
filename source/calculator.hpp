#pragma once
#include <cassert>
#include <cmath>
#include <functional>
#include <map>
#include <memory>
#include <sstream>
#include <vector>

namespace calc {

class CalculationError : public std::exception {
  std::string Msg;

public:
  CalculationError(std::string Msg) : Msg(std::move(Msg)) {}

  const char *what() const noexcept { return Msg.c_str(); }
};

enum TokenKind { TK_None = 0, TK_String = -1, TK_Number = -2, TK_END = -3 };

struct Token {
  int Kind;

  std::string Str{};

  bool operator==(std::string const &RHS) const noexcept {
    return Kind == TK_String && Str == RHS;
  }
  bool operator==(int RHS) const noexcept { return Kind == RHS; }

  double toNumber() const {
    std::istringstream SS(Str);
    double N;
    SS >> N;
    return N;
  }

  std::string getDescription() const {
    if (Kind == TK_String)
      return Str;
    if (Kind == TK_Number)
      return Str;
    if (Kind > 0)
      return std::string() + static_cast<char>(Kind);
    if (Kind == TK_END)
      return "<END>";
    return "<Err>";
  }
};

class Calculator;

class AST {
public:
  virtual ~AST() = default;
  virtual double eval(Calculator *) = 0;
};

class IdentifierAST : public AST {
  std::string Name;

public:
  explicit IdentifierAST(std::string Name) : Name(std::move(Name)) {}

  double eval(Calculator *C) final;
  const std::string &getName() const & { return Name; }
};

class UnaryExprAST : public AST {
  std::unique_ptr<AST> Operand;
  char Op;

public:
  UnaryExprAST(std::unique_ptr<AST> Operand, char Op) noexcept
      : Operand(std::move(Operand)), Op(Op) {}

  double eval(Calculator *C) final {
    if (Op == '-') {
      return -Operand->eval(C);
    }
    throw CalculationError(std::string("Unknown operator: ") + Op);
  }
};

class BinExprAST : public AST {
  std::unique_ptr<AST> LHS, RHS;
  char Op;

public:
  BinExprAST(std::unique_ptr<AST> LHS, std::unique_ptr<AST> RHS,
             char Op) noexcept
      : LHS(std::move(LHS)), RHS(std::move(RHS)), Op(Op) {}

  double eval(Calculator *C) final;
};

class ConstExpr : public AST {
  double Value;

public:
  explicit ConstExpr(double Value) noexcept : Value(Value) {}

  double eval(Calculator *C) noexcept final { return Value; }
};

class CallExprAST : public AST {
  std::string Name;
  std::vector<std::unique_ptr<AST>> Args;

public:
  CallExprAST(std::string Name, std::vector<std::unique_ptr<AST>> Args)
      : Name(std::move(Name)), Args(std::move(Args)) {}

  double eval(Calculator *C) final;

  std::vector<std::string> getParams() const;

  std::string const &getName() const &noexcept { return Name; }
};

class Calculator {

public:
  Calculator() {
    FunctionScopes.emplace_back();
    VariableScopes.emplace_back();
  }
  using Function = std::function<double(Calculator *, std::vector<double>)>;

  std::vector<std::map<std::string, Function>> FunctionScopes;

  std::vector<std::map<std::string, double>> VariableScopes;

  struct ScopeGuard {
    Calculator *Calc;

    explicit ScopeGuard(Calculator *C) noexcept : Calc(C) {}

    ~ScopeGuard() {
      Calc->VariableScopes.pop_back();
      Calc->FunctionScopes.pop_back();
    }
  };

  auto createScope() {
    VariableScopes.emplace_back();
    FunctionScopes.emplace_back();
    return ScopeGuard(this);
  }

  double calculate(std::string Expr);

  double getValue(const std::string &Name) const {
    if (auto V = findVariable(Name))
      return *V;
    throw CalculationError("No such variable: " + Name);
  }

  void setValue(const std::string &Name, double Value) {
    if (auto V = findVariable(Name))
      const_cast<double &>(*V) = Value;
    VariableScopes.back()[Name] = Value;
  }

  void setLocalVar(const std::string &Name, double Value) {
    VariableScopes.back()[Name] = Value;
  }

  Function const &getFunction(const std::string &Name) const & {
    static Function Empty;
    if (auto F = findFunction(Name))
      return *F;
    return Empty;
  }

  template <typename Callable>
  void setFunction(const std::string &Name, Callable &&Func) {
    if (auto F = findFunction(Name))
      const_cast<Function &>(*F) = std::forward<Callable>(Func);
    FunctionScopes.back()[Name] = std::forward<Callable>(Func);
  }

  std::vector<std::string> getCompletionList(const std::string &Text) const;

private:
  const double *findVariable(const std::string &Name) const noexcept {
    for (auto Scope = VariableScopes.crbegin(); Scope != VariableScopes.crend();
         ++Scope) {
      auto V = Scope->find(Name);
      if (V != Scope->cend())
        return &V->second;
    }
    return nullptr;
  }

  const Function *findFunction(const std::string &Name) const noexcept {
    for (auto Scope = FunctionScopes.crbegin(); Scope != FunctionScopes.crend();
         ++Scope) {
      auto F = Scope->find(Name);
      if (F != Scope->cend())
        return &F->second;
    }
    return nullptr;
  }
};
} // namespace calc
