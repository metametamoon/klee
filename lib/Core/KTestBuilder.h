#ifndef KLEE_KTESTBUILDER_H
#define KLEE_KTESTBUILDER_H

#include "ConstantAddressSpace.h"
#include "ExecutionState.h"
#include "Memory.h"

#include <klee/ADT/KTest.h>

#include <unordered_map>

namespace klee {

class KTestBuilder {
public:
  KTestBuilder(const ExecutionState &state, const Assignment &model);

  KTestBuilder &constructPointerGraph();
  KTestBuilder &fillContent();
  KTestBuilder &fillFinalContent();

  KTest build();

private:
  void initialize();

private:
  const ExecutionState &state_;
  const Assignment &model_;

  ConstantAddressSpace constantAddressSpace_;
  ConstantPointerGraph constantPointerGraph_;
  std::unordered_map<const MemoryObject *, std::size_t> order_;

  // Constructing object
  KTest ktest_;
};

} // namespace klee

#endif
