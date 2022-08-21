#include "test.h"

int x = 1;

int foo() {
  x += 5;
  return x;
}

int main() {
  int a = 1;

  ASSERT(2, a += 1);
  a += 4;
  ASSERT(6, a);

  x += foo() + 1;
  ASSERT(13, x);

  a -= 3;
  ASSERT(3, a);

  a *= 4;
  ASSERT(12, a);

  a /= 5;
  ASSERT(2, a);

  printf("\nOK\n");
  return 0;
}
