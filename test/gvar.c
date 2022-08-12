#include "test.h"

int a;
int b[2];
int c;
int c = 3;
int d = 2 + 3;
char e[] = "abc";
int *f = &a;
char *h = e + 3;

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

  ASSERT(4, sizeof(e));
  ASSERT(97, e[0]);
  ASSERT(0, e[3]);
  ASSERT(8, sizeof(f));
  ASSERT(1, *f);
  ASSERT(8, sizeof(h));
  ASSERT(97, *(h - 3));
  ASSERT(0, *h);

  printf("\nOK\n");
  return 0;
}
