#include "klee/Expr/SymbolicSource.h"

#include <vector>

namespace klee {

bool NoExternals(std::vector<const Array *> &arrays) {
  for (auto array : arrays) {
    if (dyn_cast<ExternalCallSource>(array->source)) {
      return false;
    }
  }
  return true;
}

bool NoExternals(std::set<const Array *> &arrays) {
  for (auto array : arrays) {
    if (dyn_cast<ExternalCallSource>(array->source)) {
      return false;
    }
  }
  return true;
}

bool ExternalsAreAssigned(std::vector<const Array *> &arrays,
                          const Assignment &assign) {
  for (auto array : arrays) {
    if (auto s = dyn_cast<ExternalCallSource>(array->source)) {
      if (s->symcrete && !assign.bindings.count(array)) {
        return false;
      }
    }
  }
  return true;
}

}
