#ifndef KLEE_PDRLEMMASSUMMARY_H
#define KLEE_PDRLEMMASSUMMARY_H

#include "ProofObligation.h"
#include "klee/Expr/Disjunction.h"
#include "klee/Expr/Expr.h"
#include "klee/Expr/ExprHashMap.h"
#include <klee/Module/KInstruction.h>
#include <optional>
#include <utility>

namespace klee {
struct KInstruction;
class ProofObligation;

;


ref<Expr> disjunctionToExpr(const disjunction &dj);
std::string disjunctionToString(const disjunction &dj);
ref<Expr> cnfToExpr(const cnf &formula);
constexpr int INF_LEVEL = std::numeric_limits<int>::max();
std::string levelToString(int level);

class PdrSummary {
public:
  PdrSummary() = default;
  void addInfinityLemmaOnSomeEdgeToPob(ProofObligation *pob,
                                       const disjunction &lemma);
  disjunction
  getInfinityLemmasFromEdgesToPob(ProofObligation *pob);

  void addLemmaOnKInstruction(KInstruction *, int level,
                              const disjunction &lemma);

  // is it true that each level has only lemma? I have no idea
  std::map<int, cnf> getLemmasFromKInstruction(KInstruction *);
  // clears maps of dead pobs
  void pobDied(ProofObligation *);

  void dumpInfinityLevelLemmas(std::string const&);

  std::map<KInstruction *, std::map<int, cnf>, KInstructionCompare>
      kinstructionLemmas;

  // from pob id
  // infinity lemmas on edges to pobs;
  // after visiting all the edges, we can create an instruction lemma on the
  // same location; some of the edges create infinity lemmas (and are stored
  // here), some of the edges generatу finite level lemmas, they will be
  // collected in the loop over edges, described in `executeLemmaUpdateAction`
  std::map<std::uint32_t, disjunction> infinityLemmas;

  std::string logPrefixWithSpace = "[pdrLemmasSummary] ";
};

} // namespace klee

#endif // KLEE_PDRLEMMASSUMMARY_H
