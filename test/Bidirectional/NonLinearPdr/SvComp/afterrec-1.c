// RUN: rm -rf test-suite
// RUN: %kleef --nonlinear-pdr --bidirectional --output-dir=%t.klee-out --property-file=%S/unreach-call.prp --max-memory=15000000000 --max-cputime-soft=900 --32 %s &>%t.log
// RUN: FileCheck %s -input-file=%t.log
// CHECK: [TRUE POSITIVE]

extern void abort(void);
extern void __assert_fail(const char *, const char *, unsigned int, const char *) __attribute__ ((__nothrow__ , __leaf__)) __attribute__ ((__noreturn__));
void reach_error() { __assert_fail("0", "afterrec-1.c", 3, "reach_error"); }

void f(int n) {
  if (n<3) return;
  n--;
  f(n);
  ERROR: {reach_error();abort();}
}

int main(void) {
  f(4);
}
