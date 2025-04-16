#include "PdrSummary.h"

#include "ProofObligation.h"
#include "StringUtil.h"
#include "klee/Expr/ExprHashMap.h"

#include <fmt/format.h>
#include <fstream>
#include <klee/Expr/CExprWriter.h>
#include <klee/Module/KInstruction.h>
#include <klee/Support/DebugFlags.h>
#include <optional>

namespace klee {

ref<Expr> disjunctionToExpr(const disjunction &dj) {
  ref<Expr> expr = Expr::createFalse();
  for (auto &literal : dj.elements) {
    expr = OrExpr::create(expr, literal); // why not OrExpr(left, right);
  }
  return expr;
}

PathConstraints cnfToPathConstraints(const cnf &formula) {
  PathConstraints result{};
  for (auto &disjunct : formula) {
    auto expr = disjunctionToExpr(disjunct);
    result.addConstraint(expr);
  }
  return result;
}

std::string disjunctionToString(const disjunction &dj) {
  if (dj.elements.empty()) {
    return "false";
  }
  std::string result{"(\\/"};
  for (const auto &atom : dj.elements) {
    result += "\n";
    result += indentString(atom->toString(), 1);
  }
  result += ")";
  return result;
}

ref<Expr> cnfToExpr(const cnf &formula) {
  ref<Expr> expr = Expr::createTrue();
  for (auto &disjunct : formula) {
    expr = AndExpr::create(expr, disjunctionToExpr(disjunct));
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

void PdrSummary::addInfinityLemmaOnSomeEdgeToPob(ProofObligation *pob,
                                                 const disjunction &lemma) {
  if (debugConstraints.isSet(DebugPrint::Lemma)) {
    llvm::errs() << logPrefixWithSpace
                 << fmt::format("Added inf lemma at pob id={} path={}\n",
                                pob->id, pob->constraints.path().toString());
    llvm::errs() << fmt::format("{}Loc={} Pretty lemma={}\n",
                                logPrefixWithSpace, pob->location->toString(),
                                disjunctionToCExpr(lemma, false));
    llvm::errs() << fmt::format("sexpr lemma: \n{}\n",
                                disjunctionToString(lemma));
  }
  infinityLemmas[pob->id].elements.insert(lemma.elements.begin(),
                                          lemma.elements.end());
  // assert(infinityLemmas.size() > 1); - maybe 'false'
}

void PdrSummary::addLemmaOnKInstruction(KInstruction *ki, int level,
                                        const disjunction &lemma) {
  if (debugConstraints.isSet(DebugPrint::Lemma)) {
    llvm::errs() << logPrefixWithSpace
                 << fmt::format("Added lemma at ki={} level={}\n",
                                ki->toString(), levelToString(level));
    llvm::errs() << fmt::format("{}Lemma={}\n", logPrefixWithSpace,
                                disjunctionToString(lemma));
  }
  kinstructionLemmas[ki][level].insert(lemma);
}

void PdrSummary::addFunctionLemma(KFunction *kf, int level,
                                  const disjunction &lemma) {
  functionLemmas[kf][level].insert(lemma);
  if (debugConstraints.isSet(DebugPrint::Lemma)) {
    llvm::errs() << logPrefixWithSpace
                 << fmt::format("Added lemma at kf={} level={}\n",
                                kf->getName().str(), levelToString(level));
    llvm::errs() << fmt::format("{}Lemma={}\n", logPrefixWithSpace,
                                disjunctionToString(lemma));
  }
}

std::map<int, cnf> PdrSummary::getLemmasFromKInstruction(KInstruction *ki) {
  return kinstructionLemmas[ki];
}

disjunction PdrSummary::getInfinityLemmasFromEdgesToPob(ProofObligation *pob) {
  return infinityLemmas[pob->id];
}

void PdrSummary::pobDied(ProofObligation *) {}

void PdrSummary::dumpInfinityLevelLemmas(const std::string &outputPath) {
  nlohmann::json invariants = nlohmann::json::array();
  llvm::errs() << "Infinity level lemmas:\n";
  for (const auto &[ki, leveledLemmas] : kinstructionLemmas) {
    for (const auto &[level, lemmas] : leveledLemmas) {
      if (level == INF_LEVEL) {
        llvm::errs() << fmt::format("ki={} ki_loc={} lemmas=\n", ki->toString(),
                                    ki->getSourceLocationString());
        for (const auto &lemma : lemmas) {
          nlohmann::json invariant{
              {"ki", ki->toString()},
              {"line", ki->getLine()},
              {"column", ki->getColumn()},
              {"function", ki->parent->parent->getName()},
              {"c_expression",
               disjunctionToCExpr(lemma, true)}, // better to over-truthify
              {"c_expression_with_errors", disjunctionToCExpr(lemma, false)},
              {"pure_expression", disjunctionToString(lemma)}};
          invariants.push_back(invariant);
        }
        llvm::errs() << "\n";
      }
    }
  }
  create_directories(std::filesystem::path{outputPath}.parent_path());
  auto inv_string = invariants.dump(2);
  std::fstream file{outputPath, std::ios::out};
  file << inv_string;
}
} // namespace klee