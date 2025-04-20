#ifndef LOGUTILS_H
#define LOGUTILS_H
#include "klee/Expr/Constraints.h"

#include <llvm-16/llvm/Support/raw_ostream.h>

namespace klee {
inline bool isSymbolicSizeConstrantAddressRead(ref<Expr> e) {
  auto readLsb = e->hasOrderedReads();
  if (readLsb) {
    auto source = readLsb->updates.root->source;
    if (source->getKind() == SymbolicSource::SymbolicSizeConstantAddress) {
      return true;
    }
  }
  return false;
}

inline bool isMemoryConstraint(ref<Expr> e) {
  if (auto notExpr = dyn_cast<NotExpr>(e)) {
    if (auto eqExpr = dyn_cast<EqExpr>(notExpr->expr)) {
      if (isSymbolicSizeConstrantAddressRead(eqExpr->right) ||
          isSymbolicSizeConstrantAddressRead(eqExpr->left)) {
        return true;
      }
    }
  }
  return false;
}

inline void dumpConstraintSet(ConstraintSet const &cs) {
  for (auto expr : cs.cs()) {
    if (!isMemoryConstraint(expr)) {
      llvm::errs() << expr->toString() << "\n";
    }
  }
}
} // namespace klee
#endif // LOGUTILS_H
