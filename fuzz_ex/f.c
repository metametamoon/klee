#include "klee/klee.h"
#include "string.h"

int main() {
  char str[3];
  klee_make_symbolic(str, sizeof(str), "str");
  if (strrchr(str, 'd') == str) {
    if (strrchr(str, 's') == str + 1) {
      return 0;
    } else {
      return 1;
    }
  } else {
    return 2;
  }
}
