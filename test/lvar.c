#include "test.h"

int f() { return 42; }

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

  int c = f();
  ASSERT(42, c);

  char msg[] = "abc";
  ASSERT(4, sizeof(msg));
  ASSERT(97, msg[0]);
  ASSERT(98, msg[1]);
  ASSERT(99, msg[2]);
  ASSERT(0, msg[3]);

  int x[] = {1, 2, f()};
  ASSERT(12, sizeof(x));
  ASSERT(1, x[0]);
  ASSERT(2, x[1]);
  ASSERT(42, x[2]);

  printf("\nOK\n");
  return 0;
}
