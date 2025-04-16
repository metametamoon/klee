#ifndef INTERPOLATE_H
#define INTERPOLATE_H
#include "TimingSolver.h"

#include <klee/Expr/Constraints.h>
#include <klee/Expr/Disjunction.h>
#include <klee/System/Time.h>

namespace klee {

struct InterpolationTimeOut {};
struct NoInterpolantExist {};
struct Interpolant {
  disjunction interpolant;
};

using InterpolationResult =
    std::variant<NoInterpolantExist, InterpolationTimeOut, Interpolant>;

InterpolationResult interpolate(cnf lhs, PathConstraints notRhs,
                                std::unique_ptr<TimingSolver> const &solver,
                                time::Span coreSolverTimeout);

} // namespace klee
#endif // INTERPOLATE_H
