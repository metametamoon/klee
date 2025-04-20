#ifndef CONTAINSQUANTIFIERSVISITOR_H
#define CONTAINSQUANTIFIERSVISITOR_H
#include "klee/Expr/ExprVisitor.h"
namespace klee {

struct ContainsVariablesVisitor : public ExprVisitor {
  bool containsQuantifiers = false;
  Action visitVariable(const VariableExpr &) override {
    containsQuantifiers = true;
    return Action::skipChildren();
  }
};

inline bool containsQuantifiers(ref<Expr> const &e) {
  ContainsVariablesVisitor visitor;
  visitor.visit(e);
  return visitor.containsQuantifiers;
}

inline bool containsQuantifiers(PathConstraints const &pathConstraints) {
  for (auto expr : pathConstraints.cs().cs()) {
    if (containsQuantifiers(expr)) {
      return true;
    }
  }
  return false;
}

inline bool containsQuantifiers(disjunction const &disjunction) {
  for (auto expr : disjunction.elements) {
    if (containsQuantifiers(expr)) {
      return true;
    }
  }
  return false;
}
} // namespace klee
#endif // CONTAINSQUANTIFIERSVISITOR_H
