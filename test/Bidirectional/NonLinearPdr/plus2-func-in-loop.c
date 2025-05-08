// REQUIRES: geq-llvm-12.0

// RUN: %clang %s %debugflags -o %t.bc
// RUN: rm -rf %t.klee-out
// RUN: %klee --write-kqueries --output-dir=%t.klee-out --non-linear-pdr --execution-mode=bidirectional --initialize-in-join-blocks=true --function-call-reproduce=reach_error --forward-ticks=0 --backward-ticks=10 --skip-not-lazy-initialized --skip-not-symbolic-objects --debug-log=rootpob,backward,conflict,closepob,reached,init,pdr,maxcompose --debug-constraints=lemma,backward --tmp-skip-fns-in-init=false --optimize=false %t.bc 2> %t.log
// RUN: FileCheck %s -input-file=%t.log
// CHECK: [FALSE POSITIVE] FOUND FALSE POSITIVE AT


#include "klee/klee.h"
#include <assert.h>
#include <stdlib.h>

void reach_error() {
  klee_assert(0);
}


int add2(int x) {
  int r = x;
  return r;
}

void myassert(int cond) {
  if (!cond) {
    reach_error();
  }
}

int main() {
  int x;
  klee_make_symbolic(&x, sizeof(x), "x");
  klee_assume(x > 0 && x < 100000);
  int a = 0;
  while (--x) {
    a = add2(a);
  }
  myassert(a % 2 == 0);
  return 0;
}
