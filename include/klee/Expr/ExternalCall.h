#ifndef KLEE_EXTERNALCALL_H
#define KLEE_EXTERNALCALL_H

#include "klee/ADT/Ref.h"
#include "klee/Expr/Expr.h"
#include "klee/Expr/ExprHashMap.h"
#include "llvm/IR/Function.h"

#include <cstdint>
#include <vector>

namespace klee {

struct ExternalCall {

  struct MO {
    class ReferenceCounter _refCount;

    bool readOnly;

    UpdateList MOState;
    const Array *SymcretePre;
    const Array *SymcretePost;
    const Array *postArray;
    uint64_t address;

    int compare(const MO &b) const {
      if (this < &b) {
        return -1;
      } else if (this == &b) {
        return 0;
      } else {
        return 1;
      }
    }
  };

  struct Argument {
    ref<Expr> expr;
    const Array *symcrete;

    bool pointer;
    ref<MO> mo;
  };

  struct Retval {
    const Array *array;
    const Array *symcrete;
  };
  /// @brief Required by klee::ref-managed objects
  class ReferenceCounter _refCount;

  unsigned num;

  // Called function
  llvm::Function *f;

  std::vector<Argument> args;
  Retval retval;

  int compare(const ExternalCall &b) const {
    if (num < b.num) {
      return -1;
    } else if (num == b.num) {
      return 0;
    } else {
      return 1;
    }
  }
};

} // end klee namespace


#endif /* KLEE_EXTERNALCALL_H */
