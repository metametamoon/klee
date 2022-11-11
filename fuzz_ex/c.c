#include "klee/klee.h"
#include "string.h"

int main() {
  char str[2];
  klee_make_symbolic(str, sizeof(str), "str");
  char str1[2];
  strcpy(str1, str);
  if (strcmp(str1, "h") == 0) {
    return 0;
  } else {
    return 1;
  }
}
