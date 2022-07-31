#!/bin/bash
cat <<EOF | cc -xc -c -o foo.o -
int foo() { return 42; }
int plus(int x, int y) { return x + y; }
EOF

assert() {
  expected="$1"
  input="$2"

  ./9cc "$input" > tmp.s
  cc -o tmp tmp.s foo.o
  ./tmp
  actual="$?"

  if [ "$actual" = "$expected" ]; then
    echo "$input => $actual"
  else
    echo  "$input => $expected expected, but got $actual"
    exit 1
  fi
}

assert 0 'main(){ 0; }'
assert 42 'main(){ 42; }'
assert 21 'main(){ 5+20-4; }'
assert 41 'main(){  12 + 34 - 5 ; }'
assert 47 'main(){ 5+6*7; }'
assert 15 'main(){ 5*(9-6); }'
assert 4 'main(){ (3+5)/2; }'
assert 4 'main(){ -6+10; }'
assert 5 'main(){ +10+(-5); }'

assert 1 'main(){ 1 == 1; }'
assert 0 'main(){ 1 == 0; }'

assert 0 'main(){ 1 != 1; }'
assert 1 'main(){ 1 != 0; }'

assert 0 'main(){ 1 > 1; }'
assert 1 'main(){ 1 > 0; }'
assert 0 'main(){ 0 > 1; }'

assert 1 'main(){ 1 >= 1; }'
assert 1 'main(){ 1 >= 0; }'
assert 0 'main(){ 0 >= 1; }'

assert 0 'main(){ 1 < 1; }'
assert 1 'main(){ 0 < 1; }'
assert 0 'main(){ 1 < 0; }'

assert 1 'main(){ 1 <= 1; }'
assert 1 'main(){ 0 <= 1; }'
assert 0 'main(){ 1 <= 0; }'

assert 3 'main(){ a = 3; }'
assert 5 'main(){ a = 3; a + 2; }'
assert 3 'main(){ a = b = 3; }'

assert 1 'main(){ fooBar_1 = 1; }'
assert 2 'main(){ foo = 1; foo = 2; foo; }'
assert 2 'main(){ foo = 1; bar = foo + 1; bar; }'

assert 14 'main(){ a = 3; b = 5 * 6 - 8; return a + b / 2; }'
assert 5 'main(){ return  5; return 8; }'

assert 1 'main(){ ifx = 1; }'
assert 5 'main(){ if(1) 5; }'
assert 5 'main(){ if(1) 5; else 4; }'
assert 4 'main(){ if(0) 5; else 4; }'
assert 3 'main(){ if(0) 5; else if(0) 4; else 3; }'

assert 1 'main(){ whilex = 1; }'
assert 1 'main(){ while(0) 0; 1; }'
assert 0 'main(){ a = 5; while(a) a = a - 1; a; }'

assert 1 'main(){ forx = 1; }'
assert 1 'main(){ for(;0;)2; 1; }'
assert 2 'main(){ for(a=5;a>2;a=a-1)1; a; }'

assert 1 'main(){ {1;} }'
assert 2 'main(){ {1;2;} }'
assert 1 'main(){ {{1;}} }'
assert 4 'main(){ f = 0; if(f){ 1; 2; } else { 3; 4; } }'
assert 2 'main(){ f = 1; if(f){ 1; 2; } else { 3; 4; } }'
assert 10 'main(){ c=0; for(i=0;i<5;i=i+1){c=c+1;c=c+1;} c; }'

assert 42 'main(){ foo(); }'

assert 8 'main(){ plus(3, 5); }'
assert 2 'main(){ plus(a=1, a); }'
assert 3 'main(){ plus(plus(1, 1), 1); }'
assert 10 'main() { plus(1,2) + plus(3,4); }'

assert 42 'bar(){ 42; } main(){ bar(); }'

assert  2 'minus(x, y){ x - y; } main(){ minus(5, 3); }'
assert  2 'nat(n){ if(n == 1) return 1; 1 + nat(n-1); } main(){ nat(2); }'
assert  10 'nat(n){ if(n == 1) return 1; 1 + nat(n-1); } main(){ nat(10); }'
assert  0 'fib(n){ if(n <= 1) return n; fib(n-1) + fib(n-2); } main(){ fib(0); }'
assert  1 'fib(n){ if(n <= 1) return n; fib(n-1) + fib(n-2); } main(){ fib(1); }'
assert  1 'fib(n){ if(n <= 1) return n; fib(n-1) + fib(n-2); } main(){ fib(2); }'
assert  2 'fib(n){ if(n <= 1) return n; fib(n-1) + fib(n-2); } main(){ fib(3); }'
assert  55 'fib(n){ if(n <= 1) return n; fib(n-1) + fib(n-2); } main(){ fib(10); }'

assert 3 'main(){ x = 3; y = &x; *y; }'

echo OK
