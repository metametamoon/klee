//===-- IndependentSolver.cpp ---------------------------------------------===//
//
//                     The KLEE Symbolic Virtual Machine
//
// This file is distributed under the University of Illinois Open Source
// License. See LICENSE.TXT for details.
//
//===----------------------------------------------------------------------===//

#include "klee/Expr/SymbolicSource.h"
#include "llvm/Support/Casting.h"
#define DEBUG_TYPE "independent-solver"
#include "klee/Solver/Solver.h"

#include "klee/Expr/Assignment.h"
#include "klee/Expr/Constraints.h"
#include "klee/Expr/Expr.h"
#include "klee/Expr/ExprUtil.h"
#include "klee/Expr/IndependentSet.h"
#include "klee/Support/Debug.h"
#include "klee/Solver/SolverImpl.h"

#include "llvm/Support/raw_ostream.h"

#include <list>
#include <map>
#include <ostream>
#include <vector>

using namespace klee;
using namespace llvm;

class IndependentSolver : public SolverImpl {
private:
  Solver *solver;

public:
  IndependentSolver(Solver *_solver) 
    : solver(_solver) {}
  ~IndependentSolver() { delete solver; }

  bool computeTruth(const Query&, Solver::TruthResponse &res);
  bool computeValidity(const Query&, Solver::ValidityResponse &res);
  Solver::PartialValidity computePartialValidity(const Query &);
  bool computeValue(const Query&, ref<Expr> &result);
  bool computeInitialValues(const Query& query,
                            const std::vector<const Array*> &objects,
                            std::vector< std::vector<unsigned char> > &values,
                            bool &hasSolution);
  SolverRunStatus getOperationStatusCode();
  char *getConstraintLog(const Query&);
  void setCoreSolverTimeout(time::Span timeout);
};

bool IndependentSolver::computeValidity(const Query& query,
                                        Solver::ValidityResponse &res) {
  std::vector< ref<Expr> > required;
  IndependentElementSet eltsClosure =
      getIndependentConstraints(query, required);

  std::vector<Symcrete> symcretes;
  Assignment symcretization(true);

  for (auto i : query.constraints.symcretes()) {
    if (eltsClosure.wholeObjects.count(i.marker) ||
        eltsClosure.elements.count(i.marker)) {
      symcretes.push_back(i);
    }
  }

  for (auto i : query.constraints.symcretization().bindings) {
    if (eltsClosure.wholeObjects.count(i.first) ||
        eltsClosure.elements.count(i.first)) {
      symcretization.bindings.insert(i);
    }
  }

  ConstraintSet tmp(required, symcretes, symcretization);
  return solver->impl->computeValidity(Query(tmp, query.expr), 
                                       res);
}

bool IndependentSolver::computeTruth(const Query &query,
                                     Solver::TruthResponse &res) {
  std::vector< ref<Expr> > required;
  IndependentElementSet eltsClosure = 
    getIndependentConstraints(query, required);

  std::vector<Symcrete> symcretes;
  Assignment symcretization(true);

  for (auto i : query.constraints.symcretes()) {
    if (eltsClosure.wholeObjects.count(i.marker) ||
        eltsClosure.elements.count(i.marker)) {
      symcretes.push_back(i);
    }
  }
  for (auto i : query.constraints.symcretization().bindings) {
    if (eltsClosure.wholeObjects.count(i.first) ||
        eltsClosure.elements.count(i.first)) {
      symcretization.bindings.insert(i);
    }
  }

  ConstraintSet tmp(required, symcretes, symcretization);
  return solver->impl->computeTruth(Query(tmp, query.expr), 
                                    res);
}

bool IndependentSolver::computeValue(const Query& query, ref<Expr> &result) {
  std::vector< ref<Expr> > required;
  IndependentElementSet eltsClosure = 
    getIndependentConstraints(query, required);

  std::vector<Symcrete> symcretes;
  Assignment symcretization(true);

  for (auto i : query.constraints.symcretes()) {
    if (eltsClosure.wholeObjects.count(i.marker) ||
        eltsClosure.elements.count(i.marker)) {
      symcretes.push_back(i);
    }
  }
  for (auto i : query.constraints.symcretization().bindings) {
    if (eltsClosure.wholeObjects.count(i.first) ||
        eltsClosure.elements.count(i.first)) {
      symcretization.bindings.insert(i);
    }
  }

  ConstraintSet tmp(required, symcretes, symcretization);
  return solver->impl->computeValue(Query(tmp, query.expr), result);
}

// Helper function used only for assertions to make sure point created
// during computeInitialValues is in fact correct. The ``retMap`` is used
// in the case ``objects`` doesn't contain all the assignments needed.
bool assertCreatedPointEvaluatesToTrue(
    const Query &query, const std::vector<const Array *> &objects,
    std::vector<std::vector<unsigned char>> &values,
    std::map<const Array *, std::vector<unsigned char>> &retMap) {
  // _allowFreeValues is set to true so that if there are missing bytes in the
  // assigment we will end up with a non ConstantExpr after evaluating the
  // assignment and fail
  Assignment assign = Assignment(objects, values, /*_allowFreeValues=*/true);

  // Add any additional bindings.
  // The semantics of std::map should be to not insert a (key, value)
  // pair if it already exists so we should continue to use the assignment
  // from ``objects`` and ``values``.
  if (retMap.size() > 0)
    assign.bindings.insert(retMap.begin(), retMap.end());

  for (auto const &constraint : query.constraints.constraints()) {
    ref<Expr> ret = assign.evaluate(constraint);

    assert(isa<ConstantExpr>(ret) &&
           "assignment evaluation did not result in constant");
    ref<ConstantExpr> evaluatedConstraint = dyn_cast<ConstantExpr>(ret);
    if (evaluatedConstraint->isFalse()) {
      return false;
    }
  }
  ref<Expr> neg = Expr::createIsZero(query.expr);
  ref<Expr> q = assign.evaluate(neg);
  assert(isa<ConstantExpr>(q) &&
         "assignment evaluation did not result in constant");
  return cast<ConstantExpr>(q)->isTrue();
}

bool IndependentSolver::computeInitialValues(const Query& query,
                                             const std::vector<const Array*> &objects,
                                             std::vector< std::vector<unsigned char> > &values,
                                             bool &hasSolution){
  // We assume the query has a solution except proven differently
  // This is important in case we don't have any constraints but
  // we need initial values for requested array objects.
  hasSolution = true;
  // FIXME: When we switch to C++11 this should be a std::unique_ptr so we don't need
  // to remember to manually call delete
  std::list<IndependentElementSet> *factors = getAllIndependentConstraintsSets(query);

  //Used to rearrange all of the answers into the correct order
  std::map<const Array*, std::vector<unsigned char> > retMap;
  for (std::list<IndependentElementSet>::iterator it = factors->begin();
       it != factors->end(); ++it) {
    std::vector<const Array*> arraysInFactorPre;
    calculateArrayReferences(*it, arraysInFactorPre);
    std::vector<const Array*> arraysInFactor;
    for (auto array : arraysInFactorPre) {
      if (isa<MakeSymbolicSource>(array->source)) {
        arraysInFactor.push_back(array);
      }
    }
    // Going to use this as the "fresh" expression for the Query() invocation below
    // assert(it->exprs.size() >= 1 && "No null/empty factors");
    if (arraysInFactor.size() == 0){
      continue;
    }

    std::vector<Symcrete> symcretes;
    Assignment symcretization(true);

    for (auto i : query.constraints.symcretes()) {
      if (it->wholeObjects.count(i.marker) ||
          it->elements.count(i.marker)) {
        symcretes.push_back(i);
      }
    }
    for (auto i : query.constraints.symcretization().bindings) {
      if (it->wholeObjects.count(i.first) ||
          it->elements.count(i.first)) {
        symcretization.bindings.insert(i);
      }
    }
    ConstraintSet tmp(it->exprs, symcretes, symcretization);
    std::vector<std::vector<unsigned char> > tempValues;
    if (!solver->impl->computeInitialValues(Query(tmp, ConstantExpr::alloc(0, Expr::Bool)),
                                            arraysInFactor, tempValues, hasSolution)){
      values.clear();
      delete factors;
      return false;
    } else if (!hasSolution){
      values.clear();
      delete factors;
      return true;
    } else {
      assert(tempValues.size() == arraysInFactor.size() &&
             "Should be equal number arrays and answers");
      for (unsigned i = 0; i < tempValues.size(); i++){
        if (retMap.count(arraysInFactor[i])){
          // We already have an array with some partially correct answers,
          // so we need to place the answers to the new query into the right
          // spot while avoiding the undetermined values also in the array
          std::vector<unsigned char> * tempPtr = &retMap[arraysInFactor[i]];
          assert(tempPtr->size() == tempValues[i].size() &&
                 "we're talking about the same array here");
          klee::DenseSet<unsigned> * ds = &(it->elements[arraysInFactor[i]]);
          for (std::set<unsigned>::iterator it2 = ds->begin(); it2 != ds->end(); it2++){
            unsigned index = * it2;
            (* tempPtr)[index] = tempValues[i][index];
          }
        } else {
          // Dump all the new values into the array
          retMap[arraysInFactor[i]] = tempValues[i];
        }
      }
    }
  }
  for (std::vector<const Array *>::const_iterator it = objects.begin();
       it != objects.end(); it++){
    const Array * arr = * it;
    if (!retMap.count(arr)){
      // this means we have an array that is somehow related to the
      // constraint, but whose values aren't actually required to
      // satisfy the query.
      std::vector<unsigned char> ret(arr->size);
      values.push_back(ret);
    } else {
      values.push_back(retMap[arr]);
    }
  }
  // Temp
  // assert(assertCreatedPointEvaluatesToTrue(query, objects, values, retMap) && "should satisfy the equation");
  delete factors;
  return true;
}

SolverImpl::SolverRunStatus IndependentSolver::getOperationStatusCode() {
  return solver->impl->getOperationStatusCode();      
}

char *IndependentSolver::getConstraintLog(const Query& query) {
  return solver->impl->getConstraintLog(query);
}

void IndependentSolver::setCoreSolverTimeout(time::Span timeout) {
  solver->impl->setCoreSolverTimeout(timeout);
}

Solver *klee::createIndependentSolver(Solver *s) {
  return new Solver(new IndependentSolver(s));
}
