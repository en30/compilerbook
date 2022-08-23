#include "test.h"

int f_0() {
  return 5;
  return 8;
}

int f_1() {
  int ifx;
  int whilex;
  int forx;
  ifx = 1;
  return ifx;
}

int f_2() {
  if (1) 5;
}

int f_3() {
  if (1)
    5;
  else
    4;
}

int f_4() {
  if (0)
    5;
  else
    4;
}

int f_5() {
  if (0)
    5;
  else if (0)
    4;
  else
    3;
}

int f_6() {
  while (0) 0;
  1;
}

int f_7() {
  int a;
  a = 5;
  while (a) a = a - 1;
  a;
}

int f_8() {
  for (; 0;) 2;
  1;
}

int f_9() {
  int a;
  for (a = 5; a > 2; a = a - 1) 1;
  a;
}

int main() {
  ASSERT(5, f_0());
  ASSERT(1, f_1());
  ASSERT(5, f_2());
  ASSERT(5, f_3());
  ASSERT(4, f_4());
  ASSERT(3, f_5());
  ASSERT(1, f_6());
  ASSERT(0, f_7());
  ASSERT(1, f_8());
  ASSERT(2, f_9());

  int i = 0;
  for (int j = 0; j < 5; j = j + 1) i += j;
  ASSERT(10, i);
  for (int i = 0; i < 5; i = i + 1) i;
  ASSERT(10, i);

  printf("\nOK\n");
  return 0;
}
