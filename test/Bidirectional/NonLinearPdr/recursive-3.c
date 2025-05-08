// REQUIRES: geq-llvm-12.0

// RUN: %clang %s %debugflags -o %t.bc
// RUN: rm -rf %t.klee-out
// RUN: %klee --write-kqueries --output-dir=%t.klee-out --non-linear-pdr --execution-mode=bidirectional --initialize-in-join-blocks=true --function-call-reproduce=reach_error --forward-ticks=0 --backward-ticks=10 --skip-not-lazy-initialized --skip-not-symbolic-objects --debug-log=rootpob,backward,conflict,closepob,reached,init,pdr --debug-constraints=lemma,backward --tmp-skip-fns-in-init=false --optimize=false --use-independent-solver=false %t.bc 2> %t.log
// RUN: FileCheck %s -input-file=%t.log
// CHECK: [FALSE POSITIVE] FOUND FALSE POSITIVE AT


#include "klee/klee.h"
#include <assert.h>
#include <stdlib.h>

void reach_error() {
  klee_assert(0);
}


int inc(int a) {
  int b = a + 1;
  return b;
}

int T(int x){
  int t;
  if (x < 0) {
    t = x;
  }
  else {
     int y = x - 1;
     // A(x, y)
     int z = T(y);
     // B(x, y, z)
     t = inc(z);
     // C(x, y, z, t) <- INC(z, t) /\ B(x, y, z)
  }
  return t; // D(t, x),  t != x
}

int main() {
  int a; // a -> sigma, sigma > 0 /\ sigma < 100000
  // f1 != 2 * sigma
  klee_make_symbolic(&a, sizeof(a), "a");
  klee_assume(a >= 0 && a < 100000);
  // f1 != 2 * read a
  int b = T(a);
  // read b != 2 * read a
  if (b != a) {
    reach_error();
  }
  return 0;
}

