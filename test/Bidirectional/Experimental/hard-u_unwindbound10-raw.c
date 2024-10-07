// RUN: %clang %s -emit-llvm -O0 -fno-discard-value-names -c -o %t.bc
// RUN: %clang %S/klee-test-comp.c -emit-llvm -O0 -fno-discard-value-names -c -o library.bc
// RUN: llvm-link -o %t.bc library.bc %t.bc
// RUN: rm -rf %t.klee-out || echo "Did not exist"
// RUN: %klee --write-kqueries --output-dir=%t.klee-out --optimize=false --execution-mode=bidirectional --function-call-reproduce=reach_error --skip-not-lazy-initialized --skip-not-symbolic-objects --initialize-in-join-blocks=true --forward-ticks=0 --backward-ticks=5 --search=dfs --use-independent-solver=false --use-guided-search=none --debug-log=rootpob,backward,conflict,closepob,reached,init,pdr,maxcompose --debug-constraints=lemma,backward --lemma-update-ticks=0 %t.bc 2> %t.log
// RUN: FileCheck %s -input-file=%t.log
// CHECK: [FALSE POSITIVE] FOUND FALSE POSITIVE AT: Target: [%entry, reach_error]
/*
  hardware integer division program, by Manna
  returns q==A//B
  */

extern void abort(void);
extern void __assert_fail(const char *, const char *, unsigned int, const char *) __attribute__ ((__nothrow__ , __leaf__)) __attribute__ ((__noreturn__));
void reach_error() { __assert_fail("0", "hard-u.c", 8, "reach_error"); }
extern unsigned int __VERIFIER_nondet_uint(void);
extern void abort(void);
void assume_abort_if_not(int cond) {
  if(!cond) {abort();}
}
void __VERIFIER_assert(int cond) {
  if (!(cond)) {
    ERROR:
        {reach_error();}
  }
  return;
}

int counter = 0;
int main() {
  unsigned int A, B;
  unsigned int r, d, p, q;
  A = __VERIFIER_nondet_uint();
  B = __VERIFIER_nondet_uint();
  assume_abort_if_not(B >= 1);

  r = A;
  d = B;
  p = 1;
  q = 0;

  while (counter++<10) {
    __VERIFIER_assert(q == 0);
    __VERIFIER_assert(r == A);
    __VERIFIER_assert(d == B * p);
    if (!(r >= d)) break;

    d = 2 * d;
    p = 2 * p;
  }

  while (counter++<10) {
    __VERIFIER_assert(A == q*B + r);
    __VERIFIER_assert(d == B*p);

    if (!(p != 1)) break;

    d = d / 2;
    p = p / 2;
    if (r >= d) {
      r = r - d;
      q = q + p;
    }
  }

  __VERIFIER_assert(A == d*q + r);
  __VERIFIER_assert(B == d);
  return 0;
}
