#ifndef KLEE_KTESTBUILDER_H
#define KLEE_KTESTBUILDER_H

#include "ConstantAddressSpace.h"
#include "ExecutionState.h"
#include "Memory.h"

#include <klee/ADT/KTest.h>

#include <unordered_map>

namespace klee {

class ExecutionState;

class KTestBuilder {
public:
  KTestBuilder(const ExecutionState &state, const Assignment &model);

  KTestBuilder &fillArgcArgv(unsigned argc, char **argv, unsigned symArgc,
                             unsigned symArgv);
  KTestBuilder &fillPointer();
  KTestBuilder &fillInitialContent();
  KTestBuilder &fillFinalContent();

  KTest build();

private:
  void initialize(const ExecutionState &state);

private:
  const Assignment &model_;

  std::vector<Symbolic> symbolics;
  ConstantAddressSpace constantAddressSpace_;
  ConstantPointerGraph constantPointerGraph_;
  std::unordered_map<const MemoryObject *, std::size_t> order_;

  // Constructing object
  KTest ktest_;
};

} // namespace klee

#endif
