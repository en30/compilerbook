#include "test.h"

int main() {
  int x;
  ASSERT(4, sizeof(1));
  ASSERT(4, sizeof(sizeof(1)));
  ASSERT(4, sizeof(x));
  ASSERT(4, sizeof(x + 3));
  ASSERT(8, sizeof(&x));

  printf("\nOK\n");
  return 0;
}
