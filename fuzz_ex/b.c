#include "klee/klee.h"
#include "string.h"

int main() {
  char str[4];
  klee_make_symbolic(str, sizeof(str), "str");
  if (strchr(str, 'd') == str) {
    return 0;
  } else {
    return 1;
  }
}
