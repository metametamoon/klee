#include "PdrLemmasSummary.h"

#include "ProofObligation.h"
#include "klee/Expr/ExprHashMap.h"

#include <fmt/format.h>
#include <fstream>
#include <klee/Module/KInstruction.h>
#include <klee/Support/DebugFlags.h>
#include <optional>

namespace klee {

ref<Expr> disjunctionSetToExpr(const disjunction &dj) {
  ref<Expr> expr = Expr::createFalse();
  for (auto &literal : dj.elements) {
    expr = OrExpr::create(expr, literal); // why not OrExpr(left, right);
  }
  return expr;
}

std::string disjunctionSetToString(const disjunction &dj) {
  if (dj.elements.empty()) {
    return "false";
  }
  std::string result {"{"};
  bool first_atom = true;
  for (const auto &atom : dj.elements) {
    if (!first_atom) {
      result += R"( \/ )";
    }
    result += atom->toString();
    first_atom = false;
  }
  result += "}";
  return result;
}

ref<Expr> cnfVectorToExpr(const cnf &formula) {
  ref<Expr> expr = Expr::createTrue();
  for (auto &disjunct : formula) {
    expr = AndExpr::create(expr, disjunctionSetToExpr(
                                     disjunct)); // why not OrExpr(left, right);
  }
  return expr;
}

std::string levelToString(int level) {
  if (level == INF_LEVEL) {
    return "INF_LEVEL";
  } else {
    return std::to_string(level);
  }
}

void PdrLemmasSummary::addInfinityLemmaOnSomeEdgeToPob(
    ProofObligation *pob, const disjunction &lemma) {
  if (debugConstraints.isSet(DebugPrint::Lemma)) {
    llvm::errs() << logPrefixWithSpace
                 << fmt::format("Added inf lemma at pob id={} path={}\n",
                                pob->id,
                                pob->constraints.path().toString());
    llvm::errs() << fmt::format("{}Loc={} Lemma={}\n", logPrefixWithSpace,
                                pob->location->toString(),
                                disjunctionSetToString(lemma));
  }
  infinityLemmas[pob->id].elements.insert(
    lemma.elements.begin(), lemma.elements.end());
  // assert(infinityLemmas.size() > 1); - maybe 'false'
}

void PdrLemmasSummary::addLemmaOnKInstruction(KInstruction *ki, int level,
                                              const disjunction &lemma) {
  if (debugConstraints.isSet(DebugPrint::Lemma)) {
    llvm::errs() << logPrefixWithSpace
                 << fmt::format("Added lemma at ki={} level={}\n",
                                ki->toString(),
                                levelToString(level));
    llvm::errs() << fmt::format("{}Lemma={}\n", logPrefixWithSpace,
                                disjunctionSetToString(lemma));

  }
  kinstructionLemmas[ki][level].insert(lemma);
}

std::map<int, cnf>
PdrLemmasSummary::getLemmasFromKInstruction(KInstruction *ki) {
  return kinstructionLemmas[ki];
}

disjunction
PdrLemmasSummary::getInfinityLemmasFromEdgesToPob(ProofObligation *pob) {
  return infinityLemmas[pob->id];
}

void PdrLemmasSummary::pobDied(ProofObligation *) {}



} // namespace klee