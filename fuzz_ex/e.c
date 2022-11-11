#include "klee/klee.h"
#include "string.h"

int main() {
  char a[2];
  char b[3];
  klee_make_symbolic(a, sizeof(a), "a");
  klee_make_symbolic(b, sizeof(b), "b");
  strcat(b,a);
  if (strcmp(a, "a") == 0) {
    if (strcmp(b, "ba") == 0) {
      return 0;
    } else {
      return -1;
    }
  } else {
    return 1;
  }
}
