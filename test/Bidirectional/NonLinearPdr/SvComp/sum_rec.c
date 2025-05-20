// RUN: rm -rf test-suite
// RUN: %kleef --nonlinear-pdr --backwards-full-logs --bidirectional --output-dir=%t.klee-out --property-file=%S/unreach-call.prp --max-memory=15000000000 --max-cputime-soft=90 --32 %s &>%t.log
// RUN: FileCheck %s -input-file=%t.log
// CHECK: [FALSE POSITIVE]
extern unsigned int __VERIFIER_nondet_uint(void);
extern void abort(void);
extern void __assert_fail(const char *, const char *, unsigned int, const char *) __attribute__ ((__nothrow__ , __leaf__)) __attribute__ ((__noreturn__));
void reach_error() { __assert_fail("0", "sum_2x3-2.c", 3, "reach_error"); }

unsigned int sum(unsigned int n, unsigned int m) {
    if (n <= 0) {
      return m + n;
    } else {
      return sum(n - 1, m + 1);
    }
}

int main(void) {
  unsigned int a = __VERIFIER_nondet_uint();
  unsigned int b = __VERIFIER_nondet_uint();
  int result = sum(a, b);
  if (result != a + b) {
    ERROR: {reach_error();abort();}
  }
}
