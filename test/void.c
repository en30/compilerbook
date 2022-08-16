#include "test.h"

// void a; => error

void f() {}

int main() {
  void *x;
  ASSERT(8, sizeof(x));

  //int a = f() => error;
  // *x => error
  // void y; => error

  printf("\nOK\n");
  return 0;
}
