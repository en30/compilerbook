#include "test.h"

struct Elem {
  char *name;
};

int main() {
  struct {
  } nil;
  ASSERT(0, sizeof(nil));

  struct {
    int a;
    int b[2];
    char *s;
    struct {
      int c;
    } y;
  } x;
  ASSERT(24, sizeof(x));
  x.a = 1;
  x.b[0] = 5;
  x.s = "abc";
  x.y.c = 42;
  ASSERT(1, x.a);
  ASSERT(5, x.b[0]);
  ASSERT(97, x.s[0]);
  ASSERT(98, x.s[1]);
  ASSERT(99, x.s[2]);
  ASSERT(0, x.s[3]);
  ASSERT(42, x.y.c);

  struct Elem e;
  ASSERT(8, sizeof(e));

  struct Elem es[2];
  es[0].name = "abc";
  ASSERT(97, es[0].name[0]);
  ASSERT(98, es[0].name[1]);
  ASSERT(99, es[0].name[2]);
  ASSERT(0, es[0].name[3]);

  struct Elem *y = &es[0];
  ASSERT(97, y->name[0]);

  printf("\nOK\n");
  return 0;
}
