#include "NonLinearPdrSummary.h"

#include "ContainsQuantifiersVisitor.h"
#include "ExprUtil.h"
#include "PdrSummary.h"
#include "ProofObligation.h"
#include "StringUtil.h"

#include <fmt/format.h>
#include <fstream>
#include <klee/Expr/CExprWriter.h>
#include <klee/Module/KInstruction.h>
#include <klee/Support/DebugFlags.h>

namespace klee {
void NonLinearPdrSummary::addDisjunctOfLemmaOnKInstruction(
    KInstruction *ki, int level, const disjunction &lemma) {
  auto simplifiedLemma = eliminateQuantifiers(lemma);
  if (debugConstraints.isSet(DebugPrint::Lemma)) {
    llvm::errs() << logPrefixWithSpace
                 << fmt::format("Extended lemma at ki={} level={}\n",
                                ki->toString(), levelToString(level));
    llvm::errs() << fmt::format("{}Lemma={}\n", logPrefixWithSpace,
                                disjunctionToString(lemma));

    llvm::errs() << fmt::format("{}simplified lemma={}\n", logPrefixWithSpace,
                                disjunctionToString(simplifiedLemma));
  }
  assert(!containsQuantifiers(simplifiedLemma));
  auto &disjunct = kinstructionIntermediateLemmas[ki][level];
  disjunct.elements.insert(simplifiedLemma.elements.begin(),
                           simplifiedLemma.elements.end());
}

void NonLinearPdrSummary::fixLemmaOnKInstruction(KInstruction *ki, int level) {
  auto lemma = kinstructionIntermediateLemmas[ki][level];
  kinstructionLemmas[ki][level].insert(lemma);
  kinstructionIntermediateLemmas[ki][level].elements.clear();
  if (debugConstraints.isSet(DebugPrint::Lemma)) {
    llvm::errs() << logPrefixWithSpace
                 << fmt::format("Added lemma at ki={} level={}\n",
                                ki->toString(), levelToString(level));
    llvm::errs() << fmt::format("{}Lemma={}\n", logPrefixWithSpace,
                                disjunctionToString(lemma));
  }
}

std::map<int, cnf>
NonLinearPdrSummary::getLemmasFromKInstruction(KInstruction *ki) {
  return kinstructionLemmas[ki];
}

void NonLinearPdrSummary::addFunctionLemma(KFunction *kf, int level,
                                           const disjunction &lemma) {
  functionLemmas[kf][level].insert(lemma);
  if (debugConstraints.isSet(DebugPrint::Lemma)) {
    llvm::errs() << fmt::format("{}Added function lemma function={} level={}\n",
                                logPrefixWithSpace, kf->getName().str(),
                                levelToString(level));
  }
}

void NonLinearPdrSummary::addDisjunctFunctionLemma(KFunction *kf, int level,
                                                   const disjunction &lemma) {
  auto &disjunct = kfunctionsIntermediateLemmas[kf][level];
  auto simplifiedLemma = eliminateQuantifiers(lemma);
  disjunct.elements.insert(simplifiedLemma.elements.begin(), simplifiedLemma.elements.end());

  if (debugConstraints.isSet(DebugPrint::Lemma)) {
    llvm::errs() << logPrefixWithSpace
                 << fmt::format("Extended lemma at kf={} level={}\n",
                                kf->getName().str(), levelToString(level));
    llvm::errs() << fmt::format("{}Lemma={}\n", logPrefixWithSpace,
                                disjunctionToString(lemma));

    llvm::errs() << fmt::format("{}Simplified lemma={}\n", logPrefixWithSpace,
                                disjunctionToString(simplifiedLemma));
  }
}

void NonLinearPdrSummary::fixFunctionLemma(KFunction *kf, int level) {
  auto lemma = kfunctionsIntermediateLemmas[kf][level];
  functionLemmas[kf][level].insert(lemma);

  if (debugConstraints.isSet(DebugPrint::Lemma)) {
    llvm::errs() << logPrefixWithSpace
                 << fmt::format("Added lemma at kf={} level={}\n",
                                kf->getName().str(), levelToString(level));
    llvm::errs() << fmt::format("{}Lemma={}\n", logPrefixWithSpace,
                                disjunctionToString(lemma));
  }
}

cnf NonLinearPdrSummary::getFunctionOverapproximation(KFunction *kf,
                                                      int level) {
  cnf result{};
  for (auto &[lemmaLevel, lemmas] : functionLemmas[kf]) {
    if (lemmaLevel >= level) {
      result.insert(lemmas.begin(), lemmas.end());
    }
  }
  return result;
}

cnf NonLinearPdrSummary::getKInstructionOverapproximation(KInstruction *ki,
                                                          int level) {
  cnf result{};
  for (auto &[lemmaLevel, lemmas] : kinstructionLemmas[ki]) {
    if (lemmaLevel >= level) {
      result.insert(lemmas.begin(), lemmas.end());
    }
  }
  return result;
}

void NonLinearPdrSummary::dumpCurrentKiLemmas() {
  llvm::errs() << "begin lemmas dumping\n";
  for (auto [ki, subarray] : kinstructionLemmas) {
    for (auto [level, lemmas] : subarray) {
      for (auto lemma : lemmas) {
        llvm::errs() << fmt::format(
            "(lemma level={} location={}\n{})\n", levelToString(level),
            ki->toString(), indentString(disjunctionToString(lemma), 1));
      }
    }
  }
  for (auto [kf, subarray] : functionLemmas) {
    for (auto [level, lemmas] : subarray) {
      for (auto lemma : lemmas) {
        llvm::errs() << fmt::format(
            "(lemma level={} location=func {}\n{})\n", levelToString(level),
            kf->getName().str(), indentString(disjunctionToString(lemma), 1));
      }
    }
  }
}

void NonLinearPdrSummary::dumpInfinityLevelLemmas(
    std::string const &outputPath) {
  nlohmann::json invariants = nlohmann::json::array();
  llvm::errs() << "Infinity level lemmas:\n";
  for (const auto &[ki, leveledLemmas] : kinstructionLemmas) {
    for (const auto &[level, lemmas] : leveledLemmas) {
      if (level == INF_LEVEL) {
        // llvm::errs() << fmt::format("ki={} ki_loc={} lemmas=\n",
        // ki->toString(), ki->getSourceLocationString());
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
        // llvm::errs() << "\n";
      }
    }
  }
  create_directories(std::filesystem::path{outputPath}.parent_path());
  auto inv_string = invariants.dump(2);
  std::fstream file{outputPath, std::ios::out};
  file << inv_string;
}

std::map<int, cnf> NonLinearPdrSummary::getFunctionLemmas(KFunction *kf) {
  return functionLemmas[kf];
}

} // namespace klee