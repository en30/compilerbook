#include "test.h"

int plus(int a, int b) { return a + b; }

int main() {
  int a[10];
  int b[2][2];

  ASSERT(40, sizeof(a));
  ASSERT(16, sizeof(b));

  *a = 1;
  ASSERT(1, a[0]);
  *(a + 1) = 2;
  ASSERT(2, a[1]);
  int *p;
  p = a;
  ASSERT(3, *p + *(p + 1));

  a[0] = 1;
  ASSERT(1, a[0]);
  ASSERT(1, *a);
  a[1] = 2;
  ASSERT(2, a[1]);
  ASSERT(2, a[0 + 1]);
  ASSERT(2, 1 [a]);

  ASSERT(3, a[0] + a[1]);

  *(a + plus(0, 1)) = 42;
  ASSERT(42, *(a + 1));

  b[0][0] = 1;
  b[1][0] = 2;
  ASSERT(1, b[0][0]);
  ASSERT(2, b[1][0]);

  printf("\nOK\n");
  return 0;
}
