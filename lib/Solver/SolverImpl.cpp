//===-- SolverImpl.cpp ----------------------------------------------------===//
//
//                     The KLEE Symbolic Virtual Machine
//
// This file is distributed under the University of Illinois Open Source
// License. See LICENSE.TXT for details.
//
//===----------------------------------------------------------------------===//

#include "klee/Solver/Solver.h"
#include "klee/Solver/SolverImpl.h"

using namespace klee;

SolverImpl::~SolverImpl() {}

bool SolverImpl::computeValidity(const Query &query, ValidityResponse &res) {
  bool trueSuccess, falseSuccess;
  TruthResponse isTrue, isFalse;
  trueSuccess = computeTruth(query, isTrue);
  if (trueSuccess && isTrue.result) {
    res.validity = Solver::MustBeTrue;
    return true;
  }
  falseSuccess = computeTruth(query.negateExpr(), isFalse);
  if (falseSuccess && isFalse.result) {
    res.validity = Solver::MustBeFalse;
    return true;
  }
  if (trueSuccess && falseSuccess) {
    res.validity = Solver::TrueOrFalse;
    res.queryDelta = isFalse.counterexampleDelta;
    res.negatedQueryDelta = isTrue.counterexampleDelta;
    return true;
  }
  if (trueSuccess && !falseSuccess) {
    res.validity = Solver::MayBeFalse;
    res.negatedQueryDelta = isTrue.counterexampleDelta;
  }
  if (!trueSuccess && falseSuccess) {
    res.validity = Solver::MayBeTrue;
    res.queryDelta = isFalse.counterexampleDelta;
    return true;
  }
  res.validity = Solver::None;
  return true;
}

const char *SolverImpl::getOperationStatusString(SolverRunStatus statusCode) {
  switch (statusCode) {
  case SOLVER_RUN_STATUS_SUCCESS_SOLVABLE:
    return "OPERATION SUCCESSFUL, QUERY IS SOLVABLE";
  case SOLVER_RUN_STATUS_SUCCESS_UNSOLVABLE:
    return "OPERATION SUCCESSFUL, QUERY IS UNSOLVABLE";
  case SOLVER_RUN_STATUS_FAILURE:
    return "OPERATION FAILED";
  case SOLVER_RUN_STATUS_TIMEOUT:
    return "SOLVER TIMEOUT";
  case SOLVER_RUN_STATUS_FORK_FAILED:
    return "FORK FAILED";
  case SOLVER_RUN_STATUS_INTERRUPTED:
    return "SOLVER PROCESS INTERRUPTED";
  case SOLVER_RUN_STATUS_UNEXPECTED_EXIT_CODE:
    return "UNEXPECTED SOLVER PROCESS EXIT CODE";
  case SOLVER_RUN_STATUS_WAITPID_FAILED:
    return "WAITPID FAILED FOR SOLVER PROCESS";
  default:
    return "UNRECOGNIZED OPERATION STATUS";
  }
}
