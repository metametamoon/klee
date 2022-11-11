#include "klee/Expr/Assignment.h"
#include "klee/Expr/Constraints.h"
#include "klee/Expr/Expr.h"
#include "klee/Expr/ExprUtil.h"
#include "klee/Expr/IndependentSet.h"
#include "klee/Expr/SymbolicSource.h"
#include "klee/Solver/Fuzzer.h"
#include "klee/Solver/Solver.h"
#include "klee/Solver/SolverImpl.h"
#include "llvm/IR/LLVMContext.h"
#include "llvm/Support/Casting.h"
#include <algorithm>
#include <vector>

namespace klee {

class FuzzingSolver : public SolverImpl {
private:
  Solver *solver;
  Fuzzer *fuzzer;

public:
  FuzzingSolver(Solver *_solver, Fuzzer *_fuzzer)
      : solver(_solver), fuzzer(_fuzzer) {}
  ~FuzzingSolver() {
    delete solver;
    delete fuzzer;
  }

  bool computeTruth(const Query &, Solver::TruthResponse &res);
  bool computeValidity(const Query &, Solver::ValidityResponse &res);
  Solver::PartialValidity computePartialValidity(const Query &);
  bool computeValue(const Query &, ref<Expr> &result);
  bool computeInitialValues(const Query &query,
                            const std::vector<const Array *> &objects,
                            std::vector<std::vector<unsigned char>> &values,
                            bool &hasSolution);
  SolverRunStatus getOperationStatusCode();
  void setCoreSolverTimeout(time::Span timeout);

private:

  std::vector<const Array *> gatherFree(std::vector<const Array *> &);

  Query constructConcretizedQuery(const Query &);
};

std::vector<const Array *>
FuzzingSolver::gatherFree(std::vector<const Array *> &arrays) {
  std::vector<const Array *> free;
  for (auto array : arrays) {
    if (dyn_cast<MakeSymbolicSource>(array->source)) {
      free.push_back(array);
    }
  }
  return free;
}

Query FuzzingSolver::constructConcretizedQuery(const Query &query) {
  const Assignment &assign = query.constraints.symcretization();
  ConstraintSet constraints;
  for (auto s : query.constraints.symcretes()) {
    constraints.add_constraint(assign.evaluate(s.asExpr()), {});
  }
  for (auto e : query.constraints.constraints()) {
    constraints.add_constraint(assign.evaluate(e), {});
  }
  ref<Expr> expr = assign.evaluate(query.expr);
  return Query(constraints, expr);
}

bool FuzzingSolver::computeValidity(const Query &query,
                                    Solver::ValidityResponse &res) {
  auto arrays = query.gatherArrays();

  if (NoExternals(arrays)) {
    return solver->impl->computeValidity(query, res);
  }

  auto concretizedQuery = constructConcretizedQuery(query);

  Solver::ValidityResponse concretizedRes;

  if (ConstantExpr *CE = dyn_cast<ConstantExpr>(concretizedQuery.expr)) {
    concretizedRes.validity =
        CE->isTrue() ? Solver::MustBeTrue : Solver::MustBeFalse;
  } else if (!solver->impl->computeValidity(concretizedQuery, concretizedRes)) {
    res.validity = Solver::None;
    return true;
  }

  if (concretizedRes.validity == Solver::TrueOrFalse) {
    res.validity = Solver::TrueOrFalse;
    return true;
  }

  Assignment newAssign(true);
  auto free = gatherFree(arrays);

  if (concretizedRes.validity == Solver::MustBeTrue) {

    if(fuzzer->fuzz(query.negateExpr(), query.constraints.symcretization().getArrays(), free, newAssign)) {
      auto checkSet = ConstraintSet(query.constraints.constraints(), query.constraints.symcretes(), newAssign);
      auto checkQ = constructConcretizedQuery(Query(checkSet, query.negateExpr().expr));
      Solver::TruthResponse check;
      assert(solver->mayBeTrue(checkQ, check));
      assert(check.result);
      res.negatedQueryDelta = newAssign;
      res.validity = Solver::TrueOrFalse;
      return true;
    } else {
      res.validity = Solver::MayBeTrue;
      return true;
    }
  } else {
    if (fuzzer->fuzz(query, query.constraints.symcretization().getArrays(), free, newAssign)) {
      auto checkSet = ConstraintSet(query.constraints.constraints(), query.constraints.symcretes(), newAssign);
      auto checkQ = constructConcretizedQuery(Query(checkSet, query.expr));
      Solver::TruthResponse check;
      assert(solver->mayBeTrue(checkQ, check));
      assert(check.result);
      res.queryDelta = newAssign;
      res.validity = Solver::TrueOrFalse;
      return true;
     } else {
      res.validity = Solver::MayBeFalse;
      return true;
    }
  }
}

bool FuzzingSolver::computeTruth(const Query &query, Solver::TruthResponse &res) {
  auto arrays = query.gatherArrays();
  if (NoExternals(arrays)) {
    return solver->impl->computeTruth(query, res);
  } else {
    auto concretizedQueryN = constructConcretizedQuery(query.negateExpr());
    Solver::TruthResponse negateRes;
    if (!solver->mayBeTrue(concretizedQueryN, negateRes)) {
      return false;
    }
    if (negateRes.result) {
      res.result = false;
      return true;
    } else {
      Solver::TruthResponse general;
      if (solver->mustBeTrue(query, general)) {
        if (general.result) {
          res.result = true;
          return true;
        }
      }
      if (solver->mustBeFalse(query, general)) {
        if (general.result) {
          res.result = false;
          return true;
        }
      }
      Assignment newAssign(true);
      auto free = gatherFree(arrays);
      if (fuzzer->fuzz(query.negateExpr(), query.constraints.symcretization().getArrays(), free, newAssign)) {
        auto checkSet = ConstraintSet(query.constraints.constraints(), query.constraints.symcretes(), newAssign);
        auto checkQ = constructConcretizedQuery(Query(checkSet, query.negateExpr().expr));
        Solver::TruthResponse check;
        assert(solver->mayBeTrue(checkQ, check));
        assert(check.result);
        res.counterexampleDelta = newAssign;
        res.result = false;
        return true;
      }
      auto concretizedQuery = constructConcretizedQuery(query);
      return solver->impl->computeTruth(concretizedQuery, res);
      // return false;
    }
  }
}

bool FuzzingSolver::computeValue(const Query &query, ref<Expr> &result) {
  auto concretizedQuery = constructConcretizedQuery(query);
  if (auto expr = dyn_cast<ConstantExpr>(concretizedQuery.expr)) {
    result = expr;
    return true;
  }
  return solver->impl->computeValue(concretizedQuery, result);
}

bool FuzzingSolver::computeInitialValues(
    const Query &query, const std::vector<const Array *> &objects,
    std::vector<std::vector<unsigned char>> &values, bool &hasSolution) {
  auto arrays = query.gatherArrays();
  if (NoExternals(arrays)) {
    return solver->impl->computeInitialValues(query, objects, values,
                                              hasSolution);
  }

  auto concretizedQuery = constructConcretizedQuery(query);

  auto objectsInternal = concretizedQuery.gatherArrays();
  std::vector<std::vector<unsigned char>> valuesInternal;

  auto success = solver->impl->computeInitialValues(
      concretizedQuery, objectsInternal, valuesInternal, hasSolution);

  if (success && hasSolution) {
    for (unsigned i = 0; i < objects.size(); i++) {
      for (unsigned j = 0; j < objectsInternal.size(); j++) {
        if (objects[i] == objectsInternal[j]) {
          values.push_back(valuesInternal[j]);
        }
      }
    }
    return true;
  } else {
    return success;
  }
}

// Redo later
SolverImpl::SolverRunStatus FuzzingSolver::getOperationStatusCode() {
  return solver->impl->getOperationStatusCode();
}


void FuzzingSolver::setCoreSolverTimeout(time::Span timeout) {
  solver->setCoreSolverTimeout(timeout / 2);
  fuzzer->setTimeout(timeout / 2);
}


std::pair<Solver *, Fuzzer *> createFuzzingSolver(Solver *s, LLVMContext &ctx) {
  auto fuzzer = new Fuzzer(ctx, s);
  return {new Solver(new FuzzingSolver(s, fuzzer)), fuzzer};
}

}
