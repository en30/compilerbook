#include "test.h"

int a;
int b[2];
int c;
int c = 3;
int d = 5;

int g() {
  int a;
  a = 2;
  return a;
}

int main() {
  ASSERT(0, a);

  a = 1;
  ASSERT(1, a);

  ASSERT(2, g());

  ASSERT(0, b[0]);
  ASSERT(0, b[1]);
  b[0] = 2;
  b[1] = 3;
  ASSERT(2, b[0]);
  ASSERT(3, b[1]);
  ASSERT(6, a + b[0] + b[1]);

  ASSERT(c, 3);
  ASSERT(d, 5);

  printf("\nOK\n");
  return 0;
}
