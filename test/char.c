#include "test.h"

int main() {
  char a;
  char x[10];

  ASSERT(1, sizeof(a));
  ASSERT(10, sizeof(x));

  a = 1;
  ASSERT(1, a);

  x[0] = -1;
  x[1] = 2;
  int y;
  y = 4;
  ASSERT(3, x[0] + y);

  printf("\nOK\n");
  return 0;
}
