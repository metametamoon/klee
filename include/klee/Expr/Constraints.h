//===-- Constraints.h -------------------------------------------*- C++ -*-===//
//
//                     The KLEE Symbolic Virtual Machine
//
// This file is distributed under the University of Illinois Open Source
// License. See LICENSE.TXT for details.
//
//===----------------------------------------------------------------------===//

#ifndef KLEE_CONSTRAINTS_H
#define KLEE_CONSTRAINTS_H

#include "klee/Expr/Assignment.h"
#include "klee/Expr/Expr.h"

namespace klee {

class MemoryObject;

/// Resembles a set of constraints that can be passed around

class Symcrete {
public:
  const Array *marker;
  ref<Expr> symcretized;

  Symcrete(ref<Expr> e, std::string name, SymbolicSource *s) : symcretized(e) {
    auto width = e->getWidth();
    auto widthInBytes = (width % 8 == 0 ? width / 8 : width / 8 + 1);
    auto array = new Array(name, widthInBytes, s);
    marker = array;
  }

  ref<Expr> asExpr() const {
    return EqExpr::create(symcretized, marker->readWhole());
  }

  bool operator==(const Symcrete &s) const {
    return symcretized == s.symcretized && marker == s.marker;
  }
};

class ConstraintSet {
  friend class ConstraintManager;

public:
  using constraints_ty = std::vector<ref<Expr>>;
  using symcretes_ty = std::vector<Symcrete>;

  ConstraintSet(constraints_ty cs, symcretes_ty sm, Assignment as)
      : _constraints(std::move(cs)), _symcretes(std::move(sm)),
        _symcretization(std::move(as)) {}

  ConstraintSet() : _symcretization(true) {}

  // size == 0 does not mean empty, we might have some symcretes
  size_t size() const noexcept;
  bool empty() const;

  bool operator==(const ConstraintSet &b) const {
    return _constraints == b._constraints && _symcretes == b._symcretes &&
           _symcretization == b._symcretization;
  }

  void dump() const;

  void add_symcrete(Symcrete s, std::vector<unsigned char> concretization);
  void add_constraint(const ref<Expr> &e, Assignment update);

  const constraints_ty &constraints() const {
    return _constraints;
  }

  const symcretes_ty &symcretes() const {
    return _symcretes;
  }

  const Assignment &symcretization() const {
    return _symcretization;
  }

private:
  constraints_ty _constraints;
  symcretes_ty _symcretes;
  Assignment _symcretization;
};

class ExprVisitor;

/// Manages constraints, e.g. optimisation
class ConstraintManager {
public:
  /// Create constraint manager that modifies constraints
  /// \param constraints
  explicit ConstraintManager(ConstraintSet &constraints);

  /// Simplify expression expr based on constraints
  /// \param constraints set of constraints used for simplification
  /// \param expr to simplify
  /// \return simplified expression
  static ref<Expr> simplifyExpr(const ConstraintSet &constraints,
                                const ref<Expr> &expr);

  /// Add constraint to the referenced constraint set
  /// \param constraint
  void addConstraint(const ref<Expr> &constraint, const Assignment &delta);

private:
  /// Rewrite set of constraints using the visitor
  /// \param visitor constraint rewriter
  /// \return true iff any constraint has been changed
  bool rewriteConstraints(ExprVisitor &visitor);

  /// Add constraint to the set of constraints
  void addConstraintInternal(const ref<Expr> &constraint);

  ConstraintSet &constraints;
};

} // namespace klee

#endif /* KLEE_CONSTRAINTS_H */
