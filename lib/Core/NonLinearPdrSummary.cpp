#include "NonLinearPdrSummary.h"
#include "PdrSummary.h"
#include "ProofObligation.h"

#include <fmt/format.h>
#include <klee/Module/KInstruction.h>
#include <klee/Support/DebugFlags.h>

namespace klee {
void NonLinearPdrSummary::addDisjunctOfLemmaOnKInstruction(
    KInstruction *ki, int level, const disjunction &lemma) {
  if (debugConstraints.isSet(DebugPrint::Lemma)) {
    llvm::errs() << logPrefixWithSpace
                 << fmt::format("Extended lemma at ki={} level={}\n",
                                ki->toString(), levelToString(level));
    llvm::errs() << fmt::format("{}Lemma={}\n", logPrefixWithSpace,
                                disjunctionToString(lemma));
  }
  auto &disjunct = kinstructionIntermediateLemmas[ki][level];
  disjunct.elements.insert(lemma.elements.begin(), lemma.elements.end());
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
  disjunct.elements.insert(lemma.elements.begin(), lemma.elements.end());
}

void NonLinearPdrSummary::fixFunctionLemma(KFunction *kf, int level) {
  functionLemmas[kf][level].insert(kfunctionsIntermediateLemmas[kf][level]);
}

std::map<int, cnf> NonLinearPdrSummary::getFunctionLemmas(KFunction *kf) {
  return functionLemmas[kf];
}

} // namespace klee