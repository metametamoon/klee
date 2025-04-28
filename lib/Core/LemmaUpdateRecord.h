#ifndef LEMMAUPDATERECORD_H
#define LEMMAUPDATERECORD_H
#include "klee/Expr/Disjunction.h"
namespace klee {
struct LemmaUpdateRecord {
  std::variant<KInstruction *, KFunction *> location;
  int oldLevel;
  int newLevel;
  disjunction lemma;
};

} // namespace klee
#endif // LEMMAUPDATERECORD_H
