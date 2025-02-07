// RUN: %clang %s -emit-llvm %O0opt -c -fno-discard-value-names -o %t.bc
// RUN: rm -rf %t.klee-out
// RUN: %klee --write-kqueries --output-dir=%t.klee-out --max-propagations=20 --max-stack-frames=15 --execution-mode=bidirectional --tmp-skip-fns-in-init=false --initialize-in-join-blocks --function-call-reproduce=reach_error --skip-not-lazy-initialized --forward-ticks=0 --backward-ticks=5 --linear-pdr-ticks=0 --skip-not-symbolic-objects --write-xml-tests --debug-log=rootpob,backward,conflict,closepob,reached,init %t.bc 2> %t.log
// RUN: FileCheck %s -input-file=%t.log

#include "klee/klee.h"
#include <assert.h>
#include <stdlib.h>

void reach_error() {
  klee_assert(0);
}

int rec(int x, int fuel) {
  if (fuel < 0) {
    return -200;
  }
  if (x <= 0) {
    return x;
  } else {
    return 1 + rec(x - 1, fuel - 1);
  }
}

int main() {
  int m = 5;
//  klee_make_symbolic(&m, sizeof(m), "m");
//  klee_assume(m > 0 && m <= 6);
  int x = rec(m, 0);
  if (m != x) {
    reach_error();
  }
}

// CHECK: [TRUE POSITIVE] FOUND TRUE POSITIVE AT
