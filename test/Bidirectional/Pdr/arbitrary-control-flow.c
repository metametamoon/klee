// REQUIRES: geq-llvm-12.0

// RUN: %clang %s %debugflags -o %t.bc
// RUN: rm -rf %t.klee-out
// RUN: %klee --write-kqueries --output-dir=%t.klee-out --optimize=false --execution-mode=bidirectional --function-call-reproduce=reach_error --skip-not-lazy-initialized --skip-not-symbolic-objects --initialize-in-join-blocks=true --backward-ticks=5 --search=dfs --use-independent-solver=false --use-guided-search=none --debug-log=rootpob,backward,conflict,closepob,reached,init,pdr,maxcompose --debug-constraints=lemma %t.bc 2> %t.log
// RUN: FileCheck %s -input-file=%t.log

#include "klee/klee.h"
#include <assert.h>
#include <stdlib.h>

void reach_error() {
  klee_assert(0);
}

void change_by_ptr(int* arg) {
  *arg = *arg + 6;
}

int loop(int x, int y, int z) {
  unsigned res = 0;
  for (int i = 0; i < x; ++i) {
    for (int k = 0; k < y; ++k) {
      res += 4;
    }
    res -= 2;
  }
  for (int j = 0; j < z; ++j) {
    change_by_ptr(&z);
  }
  if ((res & 1) == 1) { // the % operator returns -1 on neg numbers, so the check is via 'bitwise and'
    reach_error();
    return -1;
  }
  return 1;
}

int main() {
  int x;
  klee_make_symbolic(&x, sizeof(x), "x");
  int y;
  klee_make_symbolic(&y, sizeof(y), "y");
  int z;
  klee_make_symbolic(&z, sizeof(z), "z");
  return loop(x, y, z);
}

// CHECK: [FALSE POSITIVE] FOUND FALSE POSITIVE AT
