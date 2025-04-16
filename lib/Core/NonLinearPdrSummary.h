#ifndef KLEE_NONLINEARPDRLEMMASSUMMARY_H
#define KLEE_NONLINEARPDRLEMMASSUMMARY_H

#include "ProofObligation.h"
#include "klee/Expr/Disjunction.h"
#include <klee/Module/KInstruction.h>

namespace klee {
struct KInstruction;
class ProofObligation;

class NonLinearPdrSummary {
public:
  NonLinearPdrSummary() = default;
  void addDisjunctOfLemmaOnKInstruction(KInstruction *, int level,
                                        const disjunction &lemma);
  void fixLemmaOnKInstruction(KInstruction *, int level);

  std::map<int, cnf> getLemmasFromKInstruction(KInstruction *);
  // void pobDied(ProofObligation *);

  void addFunctionLemma(KFunction *kf, int level, const disjunction &lemma);

  void addDisjunctFunctionLemma(KFunction *kf, int level,
                                const disjunction &lemma);
  void fixFunctionLemma(KFunction *kf, int level);

  std::map<int, cnf> getFunctionLemmas(KFunction *kf);

  std::map<KInstruction *, std::map<int, disjunction>, KInstructionCompare>
      kinstructionIntermediateLemmas;
  std::map<KFunction *, std::map<int, disjunction>, KFunctionCompare>
      kfunctionsIntermediateLemmas;
  std::map<KInstruction *, std::map<int, cnf>, KInstructionCompare>
      kinstructionLemmas;
  std::map<KFunction *, std::map<int, cnf>, KFunctionCompare> functionLemmas;

  // from pob id
  // infinity lemmas on edges to pobs;
  // after visiting all the edges, we can create an instruction lemma on the
  // same location; some of the edges create infinity lemmas (and are stored
  // here), some of the edges generatу finite level lemmas, they will be
  // collected in the loop over edges, described in `executeLemmaUpdateAction`
  std::map<std::uint32_t, disjunction> infinityLemmas;

  std::string logPrefixWithSpace = "[nonLinearPdrSummary] ";
};

} // namespace klee

#endif // KLEE_PDRLEMMASSUMMARY_H
