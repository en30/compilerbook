#include "test.h"

int f_0() {
  { 1; }
}

int f_1() {
  {
    1;
    2;
  }
}

int f_3() {
  {
    { 1; }
  }
}

int f_4() {
  int f;
  f = 0;
  if (f) {
    1;
    2;
  } else {
    3;
    4;
  }
}

int f_5() {
  int f;
  f = 1;
  if (f) {
    1;
    2;
  } else {
    3;
    4;
  }
}

int f_6() {
  int c;
  int i;
  c = 0;
  for (i = 0; i < 5; i = i + 1) {
    c = c + 1;
    c = c + 1;
  }
  c;
}

int main() {
  ASSERT(1, f_0());
  ASSERT(2, f_1());
  ASSERT(1, f_3());
  ASSERT(4, f_4());
  ASSERT(2, f_5());
  ASSERT(10, f_6());

  printf("\nOK\n");
  return 0;
}
