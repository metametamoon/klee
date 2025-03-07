#ifndef PATHSUMMARY_H
#define PATHSUMMARY_H
#include "SummarizerTracker.h"

#include <klee/ADT/Ref.h>
#include <klee/Expr/Expr.h>
#include <klee/Expr/Path.h>
#include <string>
namespace klee {

struct PathSummary {
  std::string functionName;
  Path path;
  ref<Expr> retValue;
  std::vector<Hole> holes;
};

} // namespace klee

#endif // PATHSUMMARY_H
