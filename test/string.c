#include "test.h"

int main() {
  ASSERT(1, sizeof(""));
  ASSERT(4, sizeof("abc"));
  ASSERT(0, ""[0]);
  ASSERT(97, "abc"[0]);
  ASSERT(98, "abc"[1]);
  ASSERT(99, "abc"[2]);
  ASSERT(0, "abc"[3]);

  printf("\nOK\n");
  return 0;
}
