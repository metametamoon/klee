// REQUIRES: geq-llvm-12.0

// RUN: %clang %s %debugflags -o %t.bc
// RUN: rm -rf %t.klee-out
// RUN: %klee --write-kqueries --output-dir=%t.klee-out --optimize=false --execution-mode=bidirectional --function-call-reproduce=reach_error --skip-not-lazy-initialized --skip-not-symbolic-objects --initialize-in-join-blocks=true --search=dfs --use-guided-search=none --debug-log=rootpob,backward,conflict,closepob,reached,init,pdr --debug-constraints=lemma --backward-ticks=5 %t.bc 2> %t.log
// RUN: FileCheck %s -input-file=%t.log

#include "klee/klee.h"
#include <assert.h>
#include <stdlib.h>

void reach_error() {
  klee_assert(0);
}

int loop(int x) {
  unsigned int res = 0;
  for (int i = 0; i < x; ++i) {
    res += 2;
  }
  if (res % 2 == 1) {
    reach_error();
    return -1;
  }
  return 1;
}

int main() {
  int a;
  klee_make_symbolic(&a, sizeof(a), "a");
//   klee_assume(a > 0 && a < 100000);
  return loop(a);
}

// CHECK: [FALSE POSITIVE] FOUND FALSE POSITIVE AT: Target: [%entry, reach_error]
