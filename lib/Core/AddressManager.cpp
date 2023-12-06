#include "AddressManager.h"

#include "MemoryManager.h"
#include "klee/Expr/Expr.h"

namespace klee {

void AddressManager::addAllocation(ref<Expr> address, IDType id) {
  assert(!bindingsAdressesToObjects.count(address));
  bindingsAdressesToObjects[address] = id;
}

void *AddressManager::allocate(ref<Expr> address, ref<Expr> size) {
  IDType id = bindingsAdressesToObjects.at(address);

  auto &objects = memory->allocatedSizes.at(id);
  auto sizeLocation = objects.lower_bound(size);
  MemoryObject *newMO;
  assert(sizeLocation != objects.end());
  newMO = sizeLocation->second;
  if (newMO) {
    // assert(size <= newMO->size);
    // return reinterpret_cast<void *>(newMO->address);
    if (ref<ConstantExpr> addressExpr =
            dyn_cast<ConstantExpr>(newMO->getBaseExpr())) {
      return reinterpret_cast<void *>(addressExpr->getZExtValue());
    } else {
      return reinterpret_cast<void *>(0);
    }
  } else {
    return nullptr;
  }
}

MemoryObject *AddressManager::allocateMemoryObject(ref<Expr> address,
                                                   ref<Expr> size) {
  IDType id = bindingsAdressesToObjects.at(address);
  const auto &objects = memory->getAllocatedObjects(id);
  auto resultIterator = objects.lower_bound(size);
  if (resultIterator == objects.end()) {
    allocate(address, size);
    resultIterator = objects.lower_bound(size);
  }
  assert(resultIterator != objects.end());
  return resultIterator->second;
}

bool AddressManager::isAllocated(ref<Expr> address) {
  return bindingsAdressesToObjects.count(address);
}

} // namespace klee
