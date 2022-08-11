#include "test.h"

int main() {
  ASSERT(0, 0);
  ASSERT(42, 42);
  ASSERT(21, 5 + 20 - 4);
  ASSERT(41, 12 + 34 - 5);
  ASSERT(47, 5 + 6 * 7);
  ASSERT(15, 5 * (9 - 6));
  ASSERT(4, (3 + 5) / 2);
  ASSERT(4, -6 + 10);
  ASSERT(5, +10 + (-5));

  ASSERT(1, 1 == 1);
  ASSERT(0, 1 == 0);

  ASSERT(0, 1 != 1);
  ASSERT(1, 1 != 0);

  ASSERT(0, 1 > 1);
  ASSERT(1, 1 > 0);
  ASSERT(0, 0 > 1);

  ASSERT(1, 1 >= 1);
  ASSERT(1, 1 >= 0);
  ASSERT(0, 0 >= 1);

  ASSERT(0, 1 < 1);
  ASSERT(1, 0 < 1);
  ASSERT(0, 1 < 0);

  ASSERT(1, 1 <= 1);
  ASSERT(1, 0 <= 1);
  ASSERT(0, 1 <= 0);

  printf("\nOK\n");
  return 0;
}
