#include "test.h"

int f_0() {
  int x;
  int y;
  x = 3;
  y = &x;
  return *y;
}

int f_1() {
  int x;
  int *y;
  y = &x;
  *y = 42;
  return x;
}

int main() {
  ASSERT(3, f_0());
  ASSERT(42, f_1());

  int *p;
  alloc4(&p, 1, 2, 4, 8);
  ASSERT(1, *p);
  ASSERT(2, *(p + 1));
  ASSERT(4, *(2 + p));
  ASSERT(8, *(p + 3));
  ASSERT(1, ((p + 2) - (p + 1)));

  int *x;
  ASSERT(8, sizeof(x));
  ASSERT(8, sizeof(x + 3));
  ASSERT(4, sizeof(*x));

  int a;
  a = 42;
  ASSERT(42, *&a);
  ASSERT(42, *(&a + 1 - 1));

  printf("\nOK\n");
  return 0;
}
