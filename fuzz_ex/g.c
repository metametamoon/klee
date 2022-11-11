#include "klee/klee.h"
#include "string.h"

int main() {
  char needle[3] = "Co";
  char haystack[10];
  klee_make_symbolic(haystack, sizeof(haystack), "haystack");

  if (strstr(haystack, needle) != 0) {
    return 0;
  } else {
    return 1;
  }
}
