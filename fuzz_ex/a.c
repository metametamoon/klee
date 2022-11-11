#include "klee/klee.h"
#include "string.h"

int main() {
  char str[4] = {'a','b','c','\0'};
  int a;
  klee_make_symbolic(&a, sizeof(a), "a");
  klee_assume(a >= 0);
  klee_assume(a < 3);
  if (strlen(str + a) == 2) {
    return 0;
  } else {
    return 1;
  }
}
