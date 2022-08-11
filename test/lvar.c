#include "test.h"

int main() {
  int a;
  ASSERT(3, a = 3);
  ASSERT(5, a + 2);

  int b;
  ASSERT(3, a = b = 3);

  int fooBar_1;
  ASSERT(1, fooBar_1 = 1);

  int foo;
  foo = 1;
  foo = 2;
  ASSERT(2, foo);

  int bar;
  foo = 1;
  bar = foo + 1;

  ASSERT(2, bar);

  a = 3;
  b = 5 * 6 - 8;
  ASSERT(14, a + b / 2);

  printf("\nOK\n");
  return 0;
}
