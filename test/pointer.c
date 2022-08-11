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

int f_2() {
  int *p;
  alloc4(&p, 1, 2, 4, 8);
  return *p;
}

int f_3() {
  int *p;
  alloc4(&p, 1, 2, 4, 8);
  int *q;
  q = p + 1;
  return *q;
}

int f_4() {
  int *p;
  alloc4(&p, 1, 2, 4, 8);
  int *q;
  q = p + 2;
  return *q;
}

int f_5() {
  int *p;
  alloc4(&p, 1, 2, 4, 8);
  int *q;
  q = p + 3;
  return *q;
}

int f_6() {
  int *p;
  alloc4(&p, 1, 2, 4, 8);
  int *q;
  q = p + 3;
  q = q - 1;
  return *q;
}

int main() {
  ASSERT(3, f_0());
  ASSERT(42, f_1());

  ASSERT(1, f_2());
  ASSERT(2, f_3());
  ASSERT(4, f_4());
  ASSERT(8, f_5());
  ASSERT(4, f_6());

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
