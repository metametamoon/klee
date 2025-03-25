// REQUIRES: geq-llvm-12.0

// must pass after the trivial quantor elimination is in place
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


int f(int x) {
  int r = x + 1;
  // F1(x, r)
  int k = r + 1;
  return k;
}


int T(int x) {
  int t;
  if (x < 0) {
    t = x;
  }
  else {
     int y = x - 1;
     // A(x, y)
     int z = T(y);
     // B(x, y, z) <- T(y, z) /\ A(x, y)
     t = z + 1;
  }
  return t; // t != x
}


int main() {
  int a;
  klee_make_symbolic(&a, sizeof(a), "a");
  klee_assume(a % 2 == 0);
  int b = f(a);
  if (b % 2 != 0) {
    reach_error();
  }
  return 0;
}

