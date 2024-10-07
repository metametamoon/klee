// RUN: %clang %s -emit-llvm -O0 -fno-discard-value-names -c -o %t.bc
// RUN: %clang %S/klee-test-comp.c -emit-llvm -O0 -fno-discard-value-names -c -o library.bc
// RUN: llvm-link -o %t.bc library.bc %t.bc
// RUN: rm -rf %t.klee-out || echo "Did not exist"
// RUN: %klee --write-kqueries --output-dir=%t.klee-out --optimize=false --execution-mode=bidirectional --function-call-reproduce=reach_error --skip-not-lazy-initialized --skip-not-symbolic-objects --initialize-in-join-blocks=true --forward-ticks=0 --backward-ticks=50 --search=dfs --use-independent-solver=false --use-guided-search=none --debug-log=rootpob,backward,conflict,closepob,reached,init,pdr,maxcompose --debug-constraints=lemma,backward --lemma-update-ticks=0 %t.bc 2> %t.log
// RUN: FileCheck %s -input-file=%t.log
// CHECK: [TRUE POSITIVE]

/*
 Division algorithm from
 "Z. Manna, Mathematical Theory of Computation, McGraw-Hill, 1974"
 return x1 // x2
*/

extern void abort(void);
extern void __assert_fail(const char *, const char *, unsigned int, const char *) __attribute__ ((__nothrow__ , __leaf__)) __attribute__ ((__noreturn__));
void reach_error() { __assert_fail("0", "mannadiv.c", 9, "reach_error"); }
extern int __VERIFIER_nondet_int(void);
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
int main() {
	int counter = 0;
    int x1, x2;
    int y1, y2, y3;
    x1 = __VERIFIER_nondet_int();
    x2 = __VERIFIER_nondet_int();

    assume_abort_if_not(x1 >= 0);
    assume_abort_if_not(x2 != 0);

    y1 = 0;
    y2 = 0;
    y3 = x1;
	// while.cond
    while (counter++<1) {
		// while.body
		// 17
        __VERIFIER_assert(y1*x2 + y2 + y3 == x1);

		// 18
        if (!(y3 != 0)) break; // if.then

		// if.end
        if (y2 + 1 == x2) {
			// if.then14
            y1 = y1 + 1;
            y2 = 0;
            y3 = y3 - 1;
        } else {
			// if.else
            y2 = y2 + 1;
            y3 = y3 - 1;
        }
		// if.end18
    }
	// while.end
	// %30
    __VERIFIER_assert(y1*x2 + y2 == x1);
    return 0;
}
