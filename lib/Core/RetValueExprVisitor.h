#ifndef EXPRMAPFILTERVISITOR_H
#define EXPRMAPFILTERVISITOR_H
#include "fmt/core.h"

#include <klee/Expr/ExprVisitor.h>
#include <klee/Module/KInstruction.h>
#include <optional>

namespace klee {

// (ReadLSB w32 0 (array (w64 4) (instruction 0 %10 main -1))) ->
// Variable("Ret")
class RetValueExprVisitor : public ExprVisitor {
private:
  ref<Expr> dst;
  KInstruction *callsite;

public:
  explicit RetValueExprVisitor(KInstruction *_callsite, const ref<Expr> &_dst)
      : dst(_dst), callsite(_callsite) {}

  Action visitExpr(const Expr &e) override {
    if (isReadFromRetValue(e)) {
      return Action::changeTo(dst);
    }
    return Action::doChildren();
  }

  // Is not needed, as the expression cannot appear after replacements suddenly
  // Action visitExprPost(const Expr &e);

private:
  // (ReadLSB w32 0 (array (w64 4) (instruction 0 %10 main -1)))
  bool isReadFromRetValue(const Expr &e) {
    auto maybeSource = tryRetrieveSourceFromFullArrayRead(e);
    if (!maybeSource.has_value()) {
      return false;
    }
    auto source = *maybeSource;
    if (auto instSource = dyn_cast<InstructionSource>(source)) {
      if (instSource->index == 0 &&
          &instSource->allocSite == callsite->inst()) {
        return true;
      }
    }
    return false;
  }
  std::optional<ref<SymbolicSource>>
  tryRetrieveSourceFromFullArrayRead(Expr const &e) {
    ref<ReadExpr> base = e.hasOrderedReads(false);
    const bool isLSB = (!base.isNull());
    if (!isLSB)
      return std::nullopt;
    if (base.isNull())
      return std::nullopt;
    bool isPureRead = base->updates.head.isNull();
    if (base->index->isZero() && isPureRead) {
      return base->updates.root->source;
    } else {
      return std::nullopt;
    }
  }
};

} // namespace klee

#endif // EXPRMAPFILTERVISITOR_H
