#include "Interpolate.h"

#include "ContainsQuantifiersVisitor.h"
#include "PdrSummary.h"
#include "TimingSolver.h"

namespace klee {

std::optional<ConstraintSet> tryExcludeOne(const ConstraintSet &validityCore,
                                           ref<Expr> query,
                                           TimingSolver *solver,
                                           time::Span coreSolverTimeout) {
  for (auto exprToExclude : validityCore.cs()) {
    ConstraintSet coreWithExclusion{};
    for (auto exprToInclude : validityCore.cs()) {
      if (exprToInclude != exprToExclude) {
        coreWithExclusion.addConstraint(exprToInclude);
      }
    }
    ValidityCore core;
    bool isValid;
    solver->setTimeout(coreSolverTimeout);
    SolverQueryMetaData meta_data{};
    bool success = solver->getValidityCore(coreWithExclusion, query, core,
                                           isValid, meta_data);
    assert(success);
    if (isValid) {
      return coreWithExclusion;
    }
  }
  return std::nullopt;
}

ConstraintSet minimizeValidityCore(const ConstraintSet &validityCore,
                                   ref<Expr> query, TimingSolver *solver,
                                   time::Span coreSolverTimeout) {
  auto currentCs = validityCore;
  while (true) {
    auto maybeNewValidityCore =
        tryExcludeOne(currentCs, query, solver, coreSolverTimeout);
    if (!maybeNewValidityCore.has_value()) {
      return currentCs;
    }
    currentCs = maybeNewValidityCore.value();
  }
  assert(0 && "unreachable (the loop should be finite)");
  return ConstraintSet{};
}

InterpolationResult interpolate(const cnf &lhs,
                                const PathConstraints &negatedRhs,
                                TimingSolver *solver,
                                time::Span coreSolverTimeout) {
  if (lhs.count(disjunction{})) {
    return Interpolant{disjunction{}};
  }
  auto lhsAsConstraints = cnfToConstraintSet(lhs);
  auto addedToLhs = ExprHashSet{};
  for (auto const &constraint : negatedRhs.cs().cs()) {
    ValidityCore core;
    bool isValid;
    solver->setTimeout(coreSolverTimeout);
    SolverQueryMetaData meta_data{};
    bool success = solver->getValidityCore(lhsAsConstraints,
                                           Expr::createIsZero(constraint), core,
                                           isValid, meta_data);
    solver->setTimeout(time::Span());
    if (!success) {
      return InterpolationTimeOut{};
    }
    if (isValid) {
      disjunction interpolant;
      ConstraintSet coreCs;
      for (auto const &coreElement : core.constraints) {
        coreCs.addConstraint(coreElement);
      }
      auto optimizedCore = minimizeValidityCore(
          coreCs, Expr::createIsZero(constraint), solver, coreSolverTimeout);
      for (auto const &coreElement : optimizedCore.cs()) {
        if (addedToLhs.count(coreElement)) {
          interpolant.elements.insert(NotExpr::createIsZero(coreElement));
        }
      }
      interpolant.elements.insert(NotExpr::createIsZero(constraint));
      assert(!containsQuantifiers(interpolant));
      return Interpolant{interpolant};
    }
    lhsAsConstraints.addConstraint(constraint);
    addedToLhs.insert(constraint);
  }
  return NoInterpolantExist{};
}
} // namespace klee