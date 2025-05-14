#ifndef EXECUTIL_H

#define EXECUTIL_H
#include "klee/Expr/Constraints.h"
#include "klee/Expr/Disjunction.h"
namespace klee {

inline bool isSymbolicRead(ref<Expr> expr) {
  if (auto readLsb = expr->hasOrderedReads()) {
    auto source = readLsb->updates.root->source;
    if (source->getKind() == SymbolicSource::MakeSymbolic) {
      return true;
    }
  }
  return false;
}

inline std::optional<std::pair<ref<Expr>, ref<Expr>>>
extractReplacementFromSingleEquality(const ref<EqExpr> &eqExpr,
                                     std::optional<SummarizerTracker> tracker) {
  auto shouldBeReplaced = [&tracker](ref<Expr> expr) {
    if (auto varExpr = dyn_cast<VariableExpr>(expr)) {
      bool isInActiveHole = false;
      if (tracker.has_value()) {
        for (auto const &holes : tracker.value().holes) {
          if (expr == holes.functionRetValueSymbol) {
            isInActiveHole = true;
            break;
          }
        }
      }
      return !isInActiveHole && !varExpr->isReadFromRet;
    }
    return isSymbolicRead(expr);
  };
  if (shouldBeReplaced(eqExpr->right)) {
    return std::make_pair(eqExpr->right, eqExpr->left);
  } else if (shouldBeReplaced(eqExpr->left)) {
    return std::make_pair(eqExpr->left, eqExpr->right);
  }

  if (auto addExprRight = dyn_cast<AddExpr>(eqExpr->right)) {
    if (shouldBeReplaced(addExprRight->left)) {
      auto replacement = SubExpr::create(eqExpr->left, addExprRight->right);
      return std::make_pair(addExprRight->left, replacement);
    } else if (shouldBeReplaced(addExprRight->right)) {
      auto replacement = SubExpr::create(eqExpr->left, addExprRight->left);
      return std::make_pair(addExprRight->right, replacement);
    }
  } else if (auto addExprLeft = dyn_cast<AddExpr>(eqExpr->left)) {
    if (shouldBeReplaced(addExprLeft->left)) {
      auto replacement = SubExpr::create(eqExpr->right, addExprLeft->right);
      return std::make_pair(addExprLeft->left, replacement);
    }
    if (shouldBeReplaced(addExprLeft->right)) {
      auto replacement = SubExpr::create(eqExpr->right, addExprLeft->left);
      return std::make_pair(addExprLeft->right, replacement);
    }
  }

  return std::nullopt;
}

inline constraints_ty eliminateQuantifiers(const constraints_ty &cs) {
  constraints_ty newConstraints{};
  ExprHashMap<ref<Expr>> possibleReplacements{};
  for (auto expr : cs) {
    if (auto eqExpr = dyn_cast<EqExpr>(expr)) {
      auto singleReplacement =
          extractReplacementFromSingleEquality(eqExpr, std::nullopt);
      if (singleReplacement) {
        possibleReplacements[singleReplacement->first] =
            singleReplacement->second;
      }
    }
  }
  for (auto expr : cs) {
    auto replacedExpr = replaceExprWithReplacements(expr, possibleReplacements);
    newConstraints.insert(replacedExpr);
  }
  return newConstraints;
}

inline disjunction eliminateQuantifiers(disjunction dj) {
  constraints_ty invDj;
  for (auto atom : dj) {
    invDj.insert(Expr::createIsZero(atom));
  }
  auto invDjPrime = eliminateQuantifiers(invDj);
  disjunction newDisjunction{};
  for (auto notAtom : invDjPrime) {
    newDisjunction.elements.insert(Expr::createIsZero(notAtom));
  }
  return newDisjunction;
}
} // namespace klee
#endif // EXECUTIL_H
