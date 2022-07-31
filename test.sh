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

assert 0 'int main(){ 0; }'
assert 42 'int main(){ 42; }'
assert 21 'int main(){ 5+20-4; }'
assert 41 'int main(){  12 + 34 - 5 ; }'
assert 47 'int main(){ 5+6*7; }'
assert 15 'int main(){ 5*(9-6); }'
assert 4 'int main(){ (3+5)/2; }'
assert 4 'int main(){ -6+10; }'
assert 5 'int main(){ +10+(-5); }'

assert 1 'int main(){ 1 == 1; }'
assert 0 'int main(){ 1 == 0; }'

assert 0 'int main(){ 1 != 1; }'
assert 1 'int main(){ 1 != 0; }'

assert 0 'int main(){ 1 > 1; }'
assert 1 'int main(){ 1 > 0; }'
assert 0 'int main(){ 0 > 1; }'

assert 1 'int main(){ 1 >= 1; }'
assert 1 'int main(){ 1 >= 0; }'
assert 0 'int main(){ 0 >= 1; }'

assert 0 'int main(){ 1 < 1; }'
assert 1 'int main(){ 0 < 1; }'
assert 0 'int main(){ 1 < 0; }'

assert 1 'int main(){ 1 <= 1; }'
assert 1 'int main(){ 0 <= 1; }'
assert 0 'int main(){ 1 <= 0; }'

assert 3 'int main(){ int a; a = 3; }'
assert 5 'int main(){ int a; a = 3; a + 2; }'
assert 3 'int main(){ int a; int b; a = b = 3; }'

assert 1 'int main(){ int fooBar_1; fooBar_1 = 1; }'
assert 2 'int main(){ int foo; foo = 1; foo = 2; foo; }'
assert 2 'int main(){ int foo; int bar; foo = 1; bar = foo + 1; bar; }'

assert 14 'int main(){ int a; int b; a = 3; b = 5 * 6 - 8; return a + b / 2; }'
assert 5 'int main(){ return  5; return 8; }'

assert 1 'int main(){ int ifx; ifx = 1; }'
assert 5 'int main(){ if(1) 5; }'
assert 5 'int main(){ if(1) 5; else 4; }'
assert 4 'int main(){ if(0) 5; else 4; }'
assert 3 'int main(){ if(0) 5; else if(0) 4; else 3; }'

assert 1 'int main(){ int whilex; whilex = 1; }'
assert 1 'int main(){ while(0) 0; 1; }'
assert 0 'int main(){ int a; a = 5; while(a) a = a - 1; a; }'

assert 1 'int main(){ int forx; forx = 1; }'
assert 1 'int main(){ for(;0;)2; 1; }'
assert 2 'int main(){ int a; for(a=5;a>2;a=a-1)1; a; }'

assert 1 'int main(){ {1;} }'
assert 2 'int main(){ {1;2;} }'
assert 1 'int main(){ {{1;}} }'
assert 4 'int main(){ int f; f = 0; if(f){ 1; 2; } else { 3; 4; } }'
assert 2 'int main(){ int f; f = 1; if(f){ 1; 2; } else { 3; 4; } }'
assert 10 'int main(){ int c; int i; c=0; for(i=0;i<5;i=i+1){c=c+1;c=c+1;} c; }'

assert 42 'int main(){ foo(); }'

assert 8 'int main(){ plus(3, 5); }'
assert 2 'int main(){ int a; plus(a=1, a); }'
assert 3 'int main(){ plus(plus(1, 1), 1); }'
assert 10 'int main() { plus(1,2) + plus(3,4); }'

assert 42 'int bar(){ 42; } int main(){ bar(); }'

assert  2 'int minus(int x, int y){ x - y; } int main(){ minus(5, 3); }'
assert  2 'int nat(int n){ if(n == 1) return 1; 1 + nat(n-1); } int main(){ nat(2); }'
assert  10 'int nat(int n){ if(n == 1) return 1; 1 + nat(n-1); } int main(){ nat(10); }'
assert  0 'int fib(int n){ if(n <= 1) return n; fib(n-1) + fib(n-2); } int main(){ fib(0); }'
assert  1 'int fib(int n){ if(n <= 1) return n; fib(n-1) + fib(n-2); } int main(){ fib(1); }'
assert  1 'int fib(int n){ if(n <= 1) return n; fib(n-1) + fib(n-2); } int main(){ fib(2); }'
assert  2 'int fib(int n){ if(n <= 1) return n; fib(n-1) + fib(n-2); } int main(){ fib(3); }'
assert  55 'int fib(int n){ if(n <= 1) return n; fib(n-1) + fib(n-2); } int main(){ fib(10); }'

assert 3 'int main(){ int x; int y; x = 3; y = &x; *y; }'

assert 0 'int main() { int x; }'

echo OK
