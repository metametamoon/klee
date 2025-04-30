#ifndef SUMMARIZERTRACKER_H
#define SUMMARIZERTRACKER_H
#include <klee/ADT/Ref.h>
#include <klee/Expr/Expr.h>
#include <klee/Expr/ExprHashMap.h>

namespace klee {

struct Hole {
  std::string functionName; // or any identificator, really
  ref<VariableExpr> functionRetValueSymbol;
  std::vector<ref<Expr>> arguments; // in terms of caller; must be composed
  KInstruction *callSite;
};

struct SummarizerTracker {
public:
  // represents a hole caused by a function call within the outer function body

  ref<Expr> retValueTracker;
  std::vector<Hole> holes;
  std::vector<ExprHashMap<ref<Expr>>> reversedMappingStack;
};

} // namespace klee

#endif // SUMMARIZERTRACKER_H
