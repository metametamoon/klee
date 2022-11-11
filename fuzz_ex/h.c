#include "klee/klee.h"
#include "string.h"

int main() {
  char accepted[27] = "abcdefghijklmonpqrstuvwxyz";
  char haystack[10];
  klee_make_symbolic(haystack, sizeof(haystack), "haystack");

  if (strspn(haystack, accepted) >= 3) {
    return 0;
  } else {
    return 1;
  }
}
