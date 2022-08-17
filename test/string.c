#include "test.h"

int main() {
  ASSERT(1, sizeof(""));
  ASSERT(4, sizeof("abc"));
  ASSERT(0, ""[0]);
  ASSERT(97, "abc"[0]);
  ASSERT(98, "abc"[1]);
  ASSERT(99, "abc"[2]);
  ASSERT(0, "abc"[3]);

  ASSERT(7, "\a"[0]);
  ASSERT(8, "\b"[0]);
  ASSERT(9, "\t"[0]);
  ASSERT(10, "\n"[0]);
  ASSERT(11, "\v"[0]);
  ASSERT(12, "\f"[0]);
  ASSERT(13, "\r"[0]);
  ASSERT(27, "\e"[0]);

  ASSERT(106, "\j"[0]);
  ASSERT(107, "\k"[0]);
  ASSERT(108, "\l"[0]);

  ASSERT(7, "\ax\ny"[0]);
  ASSERT(120, "\ax\ny"[1]);
  ASSERT(10, "\ax\ny"[2]);
  ASSERT(121, "\ax\ny"[3]);

  ASSERT(0, "\0"[0]);
  ASSERT(2, sizeof("\0"));
  ASSERT(16, "\20"[0]);
  ASSERT(2, sizeof("\20"));
  ASSERT(65, "\101"[0]);
  ASSERT(2, sizeof("\101"));
  ASSERT(104, "\1500"[0]);
  ASSERT(3, sizeof("\1500"));

  ASSERT(0, "\x00"[0]);
  ASSERT(2, sizeof("\x00"));
  ASSERT(119, "\x77"[0]);
  ASSERT(2, sizeof("\x77"));
  ASSERT(-91, "\xA5"[0]);
  ASSERT(2, sizeof("\xA5"));
  ASSERT(-1, "\x00ff"[0]);
  ASSERT(2, sizeof("\x00ff"));

  printf("\nOK\n");
  return 0;
}
