//===-- Solver.cpp --------------------------------------------------------===//
//
//                     The KLEE Symbolic Virtual Machine
//
// This file is distributed under the University of Illinois Open Source
// License. See LICENSE.TXT for details.
//
//===----------------------------------------------------------------------===//

#include "klee/Solver/Solver.h"

#include "klee/Expr/Constraints.h"
#include "klee/Expr/ExprUtil.h"
#include "klee/Solver/SolverImpl.h"

using namespace klee;

const char *Solver::validity_to_str(Validity v) {
  switch (v) {
  default:    return "Unknown";
  case True:  return "True";
  case False: return "False";
  }
}

Solver::~Solver() { 
  delete impl; 
}

char *Solver::getConstraintLog(const Query& query) {
    return impl->getConstraintLog(query);
}

void Solver::setCoreSolverTimeout(time::Span timeout) {
    impl->setCoreSolverTimeout(timeout);
}

bool Solver::evaluate(const Query& query, ValidityResponse &res) {
  assert(query.expr->getWidth() == Expr::Bool && "Invalid expression type!");

  if (ConstantExpr *CE = dyn_cast<ConstantExpr>(query.expr)) {
    if (CE->isTrue()) {
      res.validity = PartialValidity::MustBeTrue;
    } else {
      res.validity = PartialValidity::MustBeFalse;
    }
    return true;
  }

  return impl->computeValidity(query, res);
}

bool Solver::mustBeTrue(const Query& query, TruthResponse &res) {
  assert(query.expr->getWidth() == Expr::Bool && "Invalid expression type!");

  // Maintain invariants implementations expect.
  if (ConstantExpr *CE = dyn_cast<ConstantExpr>(query.expr)) {
    res.result = CE->isTrue() ? true : false;
    return true;
  }

  return impl->computeTruth(query, res);
}

bool Solver::mustBeFalse(const Query& query, TruthResponse &res) {
  return mustBeTrue(query.negateExpr(), res);
}

bool Solver::mayBeTrue(const Query& query, TruthResponse &res) {
  if (!mustBeFalse(query, res))
    return false;
  res.result = !res.result;
  return true;
}

bool Solver::mayBeFalse(const Query& query, TruthResponse &res) {
  if (!mustBeTrue(query, res))
    return false;
  res.result = !res.result;
  return true;
}

bool Solver::getValue(const Query& query, ref<ConstantExpr> &result) {
  // Maintain invariants implementation expect.
  if (ConstantExpr *CE = dyn_cast<ConstantExpr>(query.expr)) {
    result = CE;
    return true;
  }

  // FIXME: Push ConstantExpr requirement down.
  ref<Expr> tmp;
  if (!impl->computeValue(query, tmp))
    return false;
  
  result = cast<ConstantExpr>(tmp);
  return true;
}

bool 
Solver::getInitialValues(const Query& query,
                         const std::vector<const Array*> &objects,
                         std::vector< std::vector<unsigned char> > &values) {
  bool hasSolution;
  bool success =
    impl->computeInitialValues(query, objects, values, hasSolution);
  // FIXME: Propogate this out.
  if (!hasSolution)
    return false;
    
  return success;
}

std::pair< ref<Expr>, ref<Expr> > Solver::getRange(const Query& query) {
  ref<Expr> e = query.expr;
  Expr::Width width = e->getWidth();
  uint64_t min, max;

  if (width==1) {
    Solver::ValidityResponse result;
    if (!evaluate(query, result))
      assert(0 && "computeValidity failed");
    switch (result.validity) {
    case Solver::MustBeTrue: 
      min = max = 1; break;
    case Solver::MustBeFalse: 
      min = max = 0; break;
    default:
      min = 0, max = 1; break;
    }
  } else if (ConstantExpr *CE = dyn_cast<ConstantExpr>(e)) {
    min = max = CE->getZExtValue();
  } else {
    // binary search for # of useful bits
    uint64_t lo=0, hi=width, mid, bits=0;
    while (lo<hi) {
      mid = lo + (hi - lo)/2;
      TruthResponse res;
      bool success = 
        mustBeTrue(query.withExpr(
                     EqExpr::create(LShrExpr::create(e,
                                                     ConstantExpr::create(mid, 
                                                                          width)),
                                    ConstantExpr::create(0, width))),
                   res);

      assert(success && "FIXME: Unhandled solver failure");
      (void) success;

      if (res.result) {
        hi = mid;
      } else {
        lo = mid+1;
      }

      bits = lo;
    }
    
    // could binary search for training zeros and offset
    // min max but unlikely to be very useful

    // check common case
    TruthResponse res;
    bool success = 
      mayBeTrue(query.withExpr(EqExpr::create(e, ConstantExpr::create(0, 
                                                                      width))), 
                res);

    assert(success && "FIXME: Unhandled solver failure");      
    (void) success;

    if (res.result) {
      min = 0;
    } else {
      // binary search for min
      lo=0, hi=bits64::maxValueOfNBits(bits);
      while (lo<hi) {
        mid = lo + (hi - lo)/2;
        TruthResponse res;
        bool success = 
          mayBeTrue(query.withExpr(UleExpr::create(e, 
                                                   ConstantExpr::create(mid, 
                                                                        width))),
                    res);

        assert(success && "FIXME: Unhandled solver failure");      
        (void) success;

        if (res.result) {
          hi = mid;
        } else {
          lo = mid+1;
        }
      }

      min = lo;
    }

    // binary search for max
    lo=min, hi=bits64::maxValueOfNBits(bits);
    while (lo<hi) {
      mid = lo + (hi - lo)/2;
      TruthResponse res;
      bool success = 
        mustBeTrue(query.withExpr(UleExpr::create(e, 
                                                  ConstantExpr::create(mid, 
                                                                       width))),
                   res);

      assert(success && "FIXME: Unhandled solver failure");      
      (void) success;

      if (res.result) {
        hi = mid;
      } else {
        lo = mid+1;
      }
    }

    max = lo;
  }

  return std::make_pair(ConstantExpr::create(min, width),
                        ConstantExpr::create(max, width));
}

std::vector<const Array *> Query::gatherArrays() const {
  std::vector<const Array *> arrays;
  ObjectFinder of(arrays);
  for (auto s : constraints.symcretes()) {
    arrays.push_back(s.marker);
    of.visit(s.symcretized);
  }
  for (auto array : constraints.constraints()) {
    of.visit(array);
  }
  of.visit(expr);
  return arrays;
}

void Query::dump() const {
  constraints.dump();

  llvm::errs() << "]\n";
  llvm::errs() << "Query [\n";
  expr->dump();
  llvm::errs() << "]\n";
}
