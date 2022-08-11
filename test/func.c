#include "test.h"

int foo() { return 42; }

int plus(int x, int y) { return x + y; }
int minus(int x, int y) { return x - y; }
int nat(int n) {
  if (n == 1) return 1;
  return 1 + nat(n - 1);
}

int fib(int n) {
  if (n <= 1) return n;
  return fib(n - 1) + fib(n - 2);
}

int main() {
  ASSERT(42, foo());
  ASSERT(8, plus(3, 5));

  int a;
  ASSERT(2, plus(a = 1, a));
  ASSERT(3, plus(plus(1, 1), 1));
  ASSERT(10, plus(1, 2) + plus(3, 4));

  ASSERT(2, minus(5, 3));

  ASSERT(2, nat(2));
  ASSERT(10, nat(10));

  ASSERT(0, fib(0));
  ASSERT(1, fib(1));
  ASSERT(1, fib(2));
  ASSERT(2, fib(3));
  ASSERT(55, fib(10));

  printf("\nOK\n");
  return 0;
}
