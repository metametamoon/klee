// REQUIRES: geq-llvm-12.0

// RUN: rm -rf %t.klee-out || echo "ok"
// RUN: %kleef --nonlinear-pdr --backwards-full-logs --bidirectional --output-dir=%t.klee-out --property-file=%S/unreach-call.prp --max-memory=15000000000 --max-cputime-soft=900 --32 %s &>%t.log
// RUN: FileCheck %s -input-file=%t.log
// CHECK: [FALSE POSITIVE] FOUND FALSE POSITIVE AT

extern void abort(void);
extern void __assert_fail(const char *, const char *, unsigned int, const char *) __attribute__ ((__nothrow__ , __leaf__)) __attribute__ ((__noreturn__));
void reach_error() { __assert_fail("0", "even.c", 3, "reach_error"); }
extern int __VERIFIER_nondet_int();
void __VERIFIER_assert(int cond) {
  if (!(cond)) {
    ERROR: {reach_error();abort();}
  }
  return;
}
int main(void) {
  unsigned int i = 0;
  unsigned int res = 0;
  while (__VERIFIER_nondet_int()) {
    res += i;
    i += i;
  }
  if ((i + res) & 1 == 1) {
    reach_error();
  }
  // __VERIFIER_assert((i + res) & 1 == 0);
  return 0;
}
