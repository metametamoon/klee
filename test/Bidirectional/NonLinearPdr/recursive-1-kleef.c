// REQUIRES: geq-llvm-12.0

// RUN: rm -rf %t.klee-out || echo "ok"
// RUN: %kleef --bidirectional --backwards-full-logs --nonlinear-pdr --output-dir=%t.klee-out --property-file=%S/unreach-call.prp --max-memory=15000000000 --max-cputime-soft=900 --32 %s &>%t.log
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



int T(int x){
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
  int a = __VERIFIER_nondet_int();
  // f1 != 2 * read a
  int b = T(a);
  // read b != 2 * read a
  if (b != a) {
    reach_error();
  }
  return 0;
}

