#ifndef _ADDRESS_MANAGER_H
#define _ADDRESS_MANAGER_H

#include "Memory.h"

#include "klee/Expr/ExprHashMap.h"

#include <cstdint>
#include <map>
#include <unordered_map>

namespace klee {
class MemoryManager;
class Array;

class AddressManager {
  friend MemoryManager;

private:
  MemoryManager *memory;
  ExprHashMap<IDType> bindingsAdressesToObjects;
  uint64_t maxSize;

public:
  AddressManager(MemoryManager *memory, uint64_t maxSize)
      : memory(memory), maxSize(maxSize) {}
  void addAllocation(ref<Expr> address, IDType id);
  void *allocate(ref<Expr> address, ref<Expr> size);
  MemoryObject *allocateMemoryObject(ref<Expr> address, ref<Expr> size);
  bool isAllocated(ref<Expr>);
};

} // namespace klee

#endif
