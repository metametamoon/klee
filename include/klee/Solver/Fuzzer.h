#ifndef KLEE_FUZZER_H
#define KLEE_FUZZER_H

#include "klee/ADT/MapOfSets.h"
#include "klee/Core/Interpreter.h"
#include "klee/Expr/Assignment.h"
#include "klee/Expr/Constraints.h"
#include "klee/Expr/Expr.h"
#include "klee/Expr/ExternalCall.h"
#include "klee/Module/KInstruction.h"
#include "klee/Solver/Solver.h"
#include "klee/System/Time.h"
#include "llvm/ExecutionEngine/ExecutionEngine.h"
#include "llvm/IR/LLVMContext.h"
#include "llvm/IR/Module.h"
#include <cstdint>
#include <memory>
#include <vector>

#include "FuzzerFFI.h"

using namespace llvm;

namespace klee {

class Fuzzer {
  LLVMContext &ctx;
  Solver *s;
  Module *singleDispatchModule;
  ExecutionEngine *executionEngine;
  InterpreterHandler *handler;
  uint64_t timeout;
  std::map<KInstruction *, int> automaton;
  MapOfSets<ref<Expr>, bool> resultCache;
  MapOfSets<ref<Expr>, Assignment> cache;

public:
  Fuzzer(LLVMContext &ctx, Solver *s);

  void initializeEngine();

  bool fuzz(const Query &q, const std::vector<const Array *> &symcretes,
            const std::vector<const Array *> &freeSymbolics,
            Assignment &assign);

  void setTimeout(time::Span _timeout);

  void dumpProgram(Module *m);

  KInstruction *current;

private:
  std::pair<std::vector<ref<Expr>>, std::vector<ref<ExternalCall>>>
  readyQuery(const Query &q);
};

} // namespace klee

#endif /* KLEE_FUZZER_H */
