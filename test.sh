#!/bin/bash
assert() {
  expected="$1"
  input="$2"

  ./9cc "$input" > tmp.s
  cc -o tmp tmp.s
  ./tmp
  actual="$?"

  if [ "$actual" = "$expected" ]; then
    echo "$input => $actual"
  else
    echo  "$input => $expected expected, but got $actual"
    exit 1
  fi
}

assert 0 "0;"
assert 42 "42;"
assert 21 "5+20-4;"
assert 41 " 12 + 34 - 5 ;"
assert 47 '5+6*7;'
assert 15 '5*(9-6);'
assert 4 '(3+5)/2;'
assert 4 '-6+10;'
assert 5 '+10+(-5);'

assert 1 '1 == 1;'
assert 0 '1 == 0;'

assert 0 '1 != 1;'
assert 1 '1 != 0;'

assert 0 '1 > 1;'
assert 1 '1 > 0;'
assert 0 '0 > 1;'

assert 1 '1 >= 1;'
assert 1 '1 >= 0;'
assert 0 '0 >= 1;'

assert 0 '1 < 1;'
assert 1 '0 < 1;'
assert 0 '1 < 0;'

assert 1 '1 <= 1;'
assert 1 '0 <= 1;'
assert 0 '1 <= 0;'

assert 3 'a = 3;'
assert 5 'a = 3; a + 2;'
assert 3 'a = b = 3;'

assert 1 'fooBar_1 = 1;'
assert 2 'foo = 1; foo = 2; foo;'
assert 2 'foo = 1; bar = foo + 1; bar;'

assert 14 'a = 3; b = 5 * 6 - 8; return a + b / 2;'
assert 5 'return  5; return 8;'

assert 1 'ifx = 1;'
assert 5 'if(1) 5;'
assert 5 'if(1) 5; else 4;'
assert 4 'if(0) 5; else 4;'
assert 3 'if(0) 5; else if(0) 4; else 3;'

assert 1 'whilex = 1;'
assert 1 'while(0) 0; 1;'
assert 0 'a = 5; while(a) a = a - 1; a;'

assert 1 'forx = 1;'
assert 1 'for(;0;)2; 1;'
assert 2 'for(a=5;a>2;a=a-1)1; a;'

echo OK
