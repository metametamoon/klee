// REQUIRES: geq-llvm-12.0

// RUN: rm -rf %t.klee-out || echo "ok"
// RUN: %kleef --nonlinear-pdr --backwards-full-logs --bidirectional --output-dir=%t.klee-out --property-file=%S/unreach-call.prp --max-memory=15000000000 --max-cputime-soft=900 --32 %s &>%t.log
// RUN: FileCheck %s -input-file=%t.log
// CHECK: [FALSE POSITIVE] FOUND FALSE POSITIVE AT

extern void abort(void);
void assume_abort_if_not(int cond) {
  if(!cond) {abort();}
}
#include <math.h>
extern void abort(void);
#include <assert.h>
void reach_error() { assert(0); }
extern double __VERIFIER_nondet_double(void);
int main()
{
  double d, q, r;
  q = __VERIFIER_nondet_double();
  assume_abort_if_not(isfinite(q));
  d=q;
  r=d+0;
  if(!(r==d)) {reach_error();abort();}
}
