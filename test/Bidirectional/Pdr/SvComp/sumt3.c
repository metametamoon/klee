// REQUIRES: geq-llvm-12.0

// RUN: rm -rf %t.klee-out || echo "ok"
// RUN: %kleef --bidirectional --output-dir=%t.klee-out --property-file=%S/unreach-call.prp --max-memory=15000000000 --max-cputime-soft=900 --32 %s &>%t.log
// RUN: FileCheck %s -input-file=%t.log
// CHECK: [FALSE POSITIVE] FOUND FALSE POSITIVE AT
// RUN: cat %t.klee-out/invs.json | grep '(((i + j) + k) == l)'


extern void abort(void);
extern void __assert_fail(const char *, const char *, unsigned int, const char *) __attribute__ ((__nothrow__ , __leaf__)) __attribute__ ((__noreturn__));
void reach_error() { __assert_fail("0", "sumt3.c", 3, "reach_error"); }
extern void abort(void);
void assume_abort_if_not(int cond) {
  if(!cond) {abort();}
}
void __VERIFIER_assert(int cond) {
  if (!(cond)) {
    ERROR: {reach_error();abort();}
  }
  return;
}
int SIZE = 20000001;
unsigned int __VERIFIER_nondet_uint();
int main() {
  unsigned int n,i,k,j,l=0;
  n = __VERIFIER_nondet_uint();
  if (!(n <= SIZE)) return 0;
  i = 0;
  j = 0;
  k = 0;
  l=0;
  while( l < n ) {

    if(!(l%3))
      i = i + 1;
    else if(!(l%2))
      j = j+1;
    else
      k = k+1;
    l = l+1;
  }
  __VERIFIER_assert((i+j+k) == l);
  return 0;
}

