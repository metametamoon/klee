//===-- Constraints.cpp ---------------------------------------------------===//
//
//                     The KLEE Symbolic Virtual Machine
//
// This file is distributed under the University of Illinois Open Source
// License. See LICENSE.TXT for details.
//
//===----------------------------------------------------------------------===//

#include "klee/Expr/Constraints.h"

#include "klee/Expr/Expr.h"
#include "klee/Expr/ExprUtil.h"
#include "klee/Expr/ExprVisitor.h"
#include "klee/Module/KModule.h"
#include "klee/Support/OptionCategories.h"

#include "llvm/IR/Function.h"
#include "llvm/Support/CommandLine.h"

#include <map>

using namespace klee;

namespace {
llvm::cl::opt<bool> RewriteEqualities(
    "rewrite-equalities",
    llvm::cl::desc("Rewrite existing constraints when an equality with a "
                   "constant is added (default=true)"),
    llvm::cl::init(true),
    llvm::cl::cat(SolvingCat));
} // namespace

class ExprReplaceVisitor : public ExprVisitor {
private:
  ref<Expr> src, dst;

public:
  ExprReplaceVisitor(const ref<Expr> &_src, const ref<Expr> &_dst)
      : src(_src), dst(_dst) {}

  Action visitExpr(const Expr &e) override {
    if (e == *src) {
      return Action::changeTo(dst);
    }
    return Action::doChildren();
  }

  Action visitExprPost(const Expr &e) override {
    if (e == *src) {
      return Action::changeTo(dst);
    }
    return Action::doChildren();
  }
};

class ExprReplaceVisitor2 : public ExprVisitor {
private:
  const std::map< ref<Expr>, ref<Expr> > &replacements;

public:
  explicit ExprReplaceVisitor2(
      const std::map<ref<Expr>, ref<Expr>> &_replacements)
      : ExprVisitor(true), replacements(_replacements) {}

  Action visitExprPost(const Expr &e) override {
    auto it = replacements.find(ref<Expr>(const_cast<Expr *>(&e)));
    if (it!=replacements.end()) {
      return Action::changeTo(it->second);
    }
    return Action::doChildren();
  }
};

bool ConstraintManager::rewriteConstraints(ExprVisitor &visitor) {
  ConstraintSet::constraints_ty old;
  bool changed = false;

  std::swap(constraints._constraints, old);
  for (auto &ce : old) {
    ref<Expr> e = visitor.visit(ce);

    if (e!=ce) {
      addConstraintInternal(e); // enable further reductions
      changed = true;
    } else {
      constraints.add_constraint(ce, {});
    }
  }

  return changed;
}

ref<Expr> ConstraintManager::simplifyExpr(const ConstraintSet &constraints,
                                          const ref<Expr> &e) {

  if (isa<ConstantExpr>(e))
    return e;

  std::map< ref<Expr>, ref<Expr> > equalities;

  for (auto &constraint : constraints._constraints) {
    if (const EqExpr *ee = dyn_cast<EqExpr>(constraint)) {
      if (isa<ConstantExpr>(ee->left)) {
        equalities.insert(std::make_pair(ee->right,
                                         ee->left));
      } else {
        equalities.insert(
            std::make_pair(constraint, ConstantExpr::alloc(1, Expr::Bool)));
      }
    } else {
      equalities.insert(
          std::make_pair(constraint, ConstantExpr::alloc(1, Expr::Bool)));
    }
  }

  return ExprReplaceVisitor2(equalities).visit(e);
}

void ConstraintManager::addConstraintInternal(const ref<Expr> &e) {
  // rewrite any known equalities and split Ands into different conjuncts

  switch (e->getKind()) {
  case Expr::Constant:
    assert(cast<ConstantExpr>(e)->isTrue() &&
           "attempt to add invalid (false) constraint");
    break;

    // split to enable finer grained independence and other optimizations
  case Expr::And: {
    BinaryExpr *be = cast<BinaryExpr>(e);
    addConstraintInternal(be->left);
    addConstraintInternal(be->right);
    break;
  }

  case Expr::Eq: {
    if (RewriteEqualities) {
      // XXX: should profile the effects of this and the overhead.
      // traversing the constraints looking for equalities is hardly the
      // slowest thing we do, but it is probably nicer to have a
      // ConstraintSet ADT which efficiently remembers obvious patterns
      // (byte-constant comparison).
      BinaryExpr *be = cast<BinaryExpr>(e);
      if (isa<ConstantExpr>(be->left)) {
	ExprReplaceVisitor visitor(be->right, be->left);
	rewriteConstraints(visitor);
      }
    }
    constraints.add_constraint(e, {});
    break;
  }

  default:
    constraints.add_constraint(e, {});
    break;
  }
}

void ConstraintManager::addConstraint(const ref<Expr> &e, const Assignment &delta) {
  for (auto i : delta.bindings) {
    constraints._symcretization.bindings[i.first] = i.second;
  }
  ref<Expr> simplified = simplifyExpr(constraints, e);
  addConstraintInternal(simplified);
}

ConstraintManager::ConstraintManager(ConstraintSet &_constraints)
    : constraints(_constraints) {}


size_t ConstraintSet::size() const {
  return _constraints.size();
}

bool ConstraintSet::empty() const {
  return _constraints.empty() && _symcretes.empty();
}

void ConstraintSet::dump() const {
  llvm::errs() << "Constraint Set:\n";
  llvm::errs() << "Constraints (" << _constraints.size() << ") [\n";
  for (const auto &constraint : _constraints) {
    constraint->dump();
  }
  llvm::errs() << "]\n";
  llvm::errs() << "Symcretes (" << _symcretes.size() << ") [\n";
  for (unsigned s = 0; s < _symcretes.size(); s++) {
    auto &symcrete = _symcretes[s];
    symcrete.symcretized->dump();
    llvm::errs() << "Concretization: ";
    auto &value = _symcretization.bindings.at(symcrete.marker);
    for (unsigned i = 0; i < value.size(); i++) {
      llvm::errs() << (unsigned int)value[i] << (i + 1 == value.size() ? "" : " ");
    }
    llvm::errs() << "\n" << (s + 1 == _symcretes.size() ? "" : "\n");
  }
  llvm::errs() << "]\n";
}

void ConstraintSet::add_symcrete(Symcrete s,
                                  std::vector<unsigned char> concretization) {
  _symcretes.push_back(std::move(s));
  _symcretization.bindings.insert({s.marker, std::move(concretization)});
}

void ConstraintSet::add_constraint(const ref<Expr> &e, Assignment update) {
  _constraints.push_back(e);
  for (auto i : update.bindings) {
    _symcretization.bindings[i.first] = i.second;
  }
}
