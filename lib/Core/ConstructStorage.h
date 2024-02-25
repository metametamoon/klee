//===-- ConstructStorage.h --------------------------------------*- C++ -*-===//
//
//                     The KLEE Symbolic Virtual Machine
//
// This file is distributed under the University of Illinois Open Source
// License. See LICENSE.TXT for details.
//
//===----------------------------------------------------------------------===//

#ifndef KLEE_CONSTRUCT_STORAGE_H
#define KLEE_CONSTRUCT_STORAGE_H

#include "klee/ADT/SparseStorage.h"
#include "klee/Expr/Expr.h"

#include <functional>

namespace klee {

template <typename ValueType, typename Eq = std::equal_to<ValueType>>
Storage<ValueType, Eq> *constructStorage(ref<Expr> size,
                                         const ValueType &defaultValue) {
  if (auto constSize = dyn_cast<ConstantExpr>(size);
      constSize && constSize->getZExtValue() <= 64) {
    return new SparseStorage<ValueType, Eq, PersistentArray<ValueType, Eq>>(
        defaultValue, typename PersistentArray<ValueType, Eq>::allocator(
                          constSize->getZExtValue()));
  } else {
    return new SparseStorage<ValueType, Eq, UnorderedMapAdapder<ValueType, Eq>>(
        defaultValue);
    // new SparseStorage<ValueType, Eq,
    //                   PersistenUnorderedMapAdapder<ValueType, Eq>>(
    //     defaultValue);
  }
}
} // namespace klee

#endif /* KLEE_CONSTRUCT_STORAGE_H */
