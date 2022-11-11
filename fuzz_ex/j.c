#include "klee/klee.h"
#include "string.h"

int main() {
  char a[] = "ab";
  char b[3];
  klee_make_symbolic(b, sizeof(b), "b");
  if (strcasecmp(a,b)) {
    if (b[0] == 'A') {
      return 0;
    } else {
      return 1;
    }
  } else {
    return 2;
  }
}
