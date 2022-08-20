#include "test.h"

int a = 1;

struct Elem {
  char *name;
};

int main() {
  ASSERT(1, a);

  int a = 2;

  {
    int a = 3;
    {
      int a = 4;
      ASSERT(4, a);
    }
    ASSERT(3, a);
  }
  ASSERT(2, a);

  struct Elem e;
  ASSERT(8, sizeof(e));

  struct Elem {
    char *name;
    int x;
  } e2;
  {
    struct Elem {
      char *name;
      int x;
      int y;
    } e3;

    ASSERT(16, sizeof(e3));
  }
  ASSERT(12, sizeof(e2));

  printf("\nOK\n");
  return 0;
}
