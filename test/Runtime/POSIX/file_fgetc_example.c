// REQUIRES: geq-llvm-10.0
// RUN: %clang %s -emit-llvm -g %O0opt -c -o %t.bc
// RUN: rm -rf %t.klee-out
// RUN: %klee --output-dir=%t.klee-out --posix-runtime %t.bc --sym-stdin 64 --sym-files 3 64
// RUN: test -f %t.klee-out/test000001.ktestjson
// RUN: test -f %t.klee-out/test000002.ktestjson
// RUN: test -f %t.klee-out/test000003.ktestjson
// RUN: test -f %t.klee-out/test000004.ktestjson
// RUN: test -f %t.klee-out/test000005.ktestjson
// RUN: test -f %t.klee-out/test000006.ktestjson
// RUN: test -f %t.klee-out/test000007.ktestjson
// RUN: test -f %t.klee-out/test000008.ktestjson
// RUN: test -f %t.klee-out/test000009.ktestjson
// RUN: test -f %t.klee-out/test000010.ktestjson
// RUN: test -f %t.klee-out/test000011.ktestjson
// RUN: test -f %t.klee-out/test000012.ktestjson
// RUN: test -f %t.klee-out/test000013.ktestjson
// RUN: not test -f %t.klee-out/test000014.ktestjson

#include <stdio.h>

int main() {
  FILE *fA = fopen("A", "r");
  FILE *fB = fopen("B", "r");
  FILE *fC = fopen("C", "r");
  unsigned char x = fgetc(fA);
  if (x >= '0' && x <= '9') {
    unsigned char a = fgetc(fB);
    if (a >= 'a' && a <= 'z') {
      return 1;
    } else {
      return 2;
    }
  } else {
    unsigned char a = fgetc(fC);
    unsigned char b = fgetc(fA);
    if (a >= 'a' && a <= 'z') {
      if (b >= '0' && b <= '9') {
        return 3;
      } else {
        return 4;
      }
    } else {
      return 5;
    }
  }
}
