#ifndef INTERPOLATE_H
#define INTERPOLATE_H
#include "TimingSolver.h"

#include <klee/Expr/Constraints.h>
#include <klee/Expr/Disjunction.h>
#include <klee/System/Time.h>
#include <variant>

namespace klee {

struct InterpolationTimeOut {};
struct NoInterpolantExist {};
struct Interpolant {
  disjunction interpolant;
};

using InterpolationResult =
    std::variant<NoInterpolantExist, InterpolationTimeOut, Interpolant>;

InterpolationResult interpolate(const cnf &lhs, const PathConstraints &notRhs,
                                TimingSolver *solver,
                                time::Span coreSolverTimeout);

} // namespace klee
#endif // INTERPOLATE_H
