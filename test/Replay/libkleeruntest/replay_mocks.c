// RUN: %clang %s -emit-llvm -g %O0opt -c -o %t.bc
// RUN: rm -rf %t.klee-out
// RUN: %klee --output-dir=%t.klee-out --external-calls=all --mock-strategy=naive %t.bc
// RUN: %runmocks %libkleeruntest -o %t.klee-out/a.out %t.klee-out %t.bc
// RUN: test -f %t.klee-out/test000001.ktest

extern int foo(int);

int main() {
  int a;
  klee_make_symbolic(&a, sizeof(a), "a");
  return foo(a);
}
