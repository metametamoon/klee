#ifndef KLEE_SOURCEBUILDER_H
#define KLEE_SOURCEBUILDER_H

#include "SymbolicSource.h"
#include "klee/Expr/ExternalCall.h"
#include <unordered_map>

namespace klee {

class SourceBuilder {
private:
  ref<SymbolicSource> constantSource;
  ref<SymbolicSource> makeSymbolicSource;
  std::map<ref<Expr>, ref<SymbolicSource>> lazyInitializations;
  std::map<std::pair<ref<ExternalCall>, bool>, ref<SymbolicSource>> externalCalls;
public:
  SourceBuilder();
  SymbolicSource *constant();
  SymbolicSource *makeSymbolic();
  SymbolicSource *lazyInitialized(ref<Expr>);
  SymbolicSource *externalCall(ref<ExternalCall>, bool);
};

};

#endif /* KLEE_EXPRBUILDER_H */
