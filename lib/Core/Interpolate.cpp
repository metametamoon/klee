#include "Interpolate.h"

#include "PdrSummary.h"
#include "TimingSolver.h"

namespace klee {
InterpolationResult interpolate(cnf lhs, PathConstraints negatedRhs,
                                std::unique_ptr<TimingSolver> const &solver,
                                time::Span coreSolverTimeout) {
  if (lhs.count(disjunction{})) {
    return Interpolant{disjunction{}};
  }
  auto lhsAsConstraints = cnfToPathConstraints(lhs);
  auto addedToLhs = ExprHashSet{};
  for (auto constraint : negatedRhs.cs().cs()) {
    ValidityCore core;
    bool isValid;
    solver->setTimeout(coreSolverTimeout);
    SolverQueryMetaData meta_data{};
    bool success = solver->getValidityCore(lhsAsConstraints.cs(),
                                           Expr::createIsZero(constraint), core,
                                           isValid, meta_data);
    solver->setTimeout(time::Span());
    if (!success) {
      return InterpolationTimeOut{};
    }
    if (isValid) {
      disjunction interpolant;
      for (auto coreElement : core.constraints) {
        if (addedToLhs.count(coreElement)) {
          interpolant.elements.insert(coreElement);
        }
      }
      interpolant.elements.insert(NotExpr::createIsZero(constraint));
      return Interpolant{interpolant};
    }
    auto addedConstraint = NotExpr::createIsZero(constraint);
    lhsAsConstraints.addConstraint(addedConstraint);
    addedToLhs.insert(constraint);
  }
  return NoInterpolantExist{};
}
} // namespace klee