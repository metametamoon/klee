// REQUIRES: geq-llvm-12.0

// RUN: rm -rf %t.klee-out || echo "ok"
// RUN: %kleef --bidirectional --output-dir=%t.klee-out --property-file=%S/unreach-call.prp --max-memory=15000000000 --max-cputime-soft=900 --32 %s &>%t.log
// RUN: FileCheck %s -input-file=%t.log
// CHECK: [FALSE POSITIVE] FOUND FALSE POSITIVE AT
// RUN: cat %t.klee-out/invs.json | grep '(0 == (x % 2))'

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
  unsigned int x = 0;
  while (__VERIFIER_nondet_int()) {
    x += 2;
  }
  __VERIFIER_assert(!(x % 2));
  return 0;
}
