#include "klee/klee.h"
#include "string.h"

int main() {
  char str[3];
  klee_make_symbolic(str, sizeof(str), "str");
  if (strcasecmp(str, "Ho")) {
    return 0;
  } else {
    return 1;
  }
}
