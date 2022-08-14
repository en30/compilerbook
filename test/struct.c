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
    int b;
    char *s;
  } x;
  ASSERT(16, sizeof(x));

  struct Elem e;
  ASSERT(8, sizeof(e));

  printf("\nOK\n");
  return 0;
}
