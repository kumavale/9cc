#!/bin/bash

assert() {
    expected="$1"
    input="$2"

    ./9cc "$input" > tmp.s
    gcc -o tmp tmp.s
    ./tmp
    actual="$?"

    if [ "$actual" = "$expected" ]; then
        echo "$input => $actual"
    else
        echo "$input => $expected expected, but got $actual"
        exit 1
    fi
}

assert 0 'return 0;'
assert 42 'return 42;'
assert 21 'return 5+20-4;'
assert 41 'return 12 + 34 - 5;'
assert 47 'return 5+6*7;'
assert 15 'return 5*(9-6);'
assert 4 'return (3+5)/2;'
assert 10 'return -10+20;'
assert 10 'return - -10;'
assert 10 'return - - +10;'

assert 0 'return 0==1;'
assert 1 'return 42==42;'
assert 1 'return 0!=1;'
assert 0 'return 42!=42;'

assert 1 'return 0<1;'
assert 0 'return 1<1;'
assert 0 'return 2<1;'
assert 1 'return 0<=1;'
assert 1 'return 1<=1;'
assert 0 'return 2<=1;'

assert 1 'return 1>0;'
assert 0 'return 1>1;'
assert 0 'return 1>2;'
assert 1 'return 1>=0;'
assert 1 'return 1>=1;'
assert 0 'return 1>=2;'

assert 1 'return 1; 2; 3;'
assert 2 '1; return 2; 3;'
assert 3 '1; 2; return 3;'

assert 3 'foo = 3; return foo;'
assert 8 'foo_123=3; bar=5; return foo_123+bar;'
assert 1 'f1=-1; f2=2; return f1+f2;'

assert 1 'if(1) return 1; return 0;'
assert 0 'if(0) return 1; return 0;'
assert 1 'if(1*2==5-3) return 1; return 0;'
assert 3 'if(1) if(2) return 3; return 4;'

assert 1 'if(1) return 1; else return 2;'
assert 2 'if(0) return 1; else if(1) return 2;'
assert 3 'if(0) return 1; else if(0) return 2; else return 3;'

assert 4 'i=5; j=0; while(i=i-1) j=j+1; return j;'
assert 1 'while(0) return 0; return 1;'

assert 10 'j=0; for(i=0; i<5; i=i+1) j=j+i; return j;'
assert 1 'for(;;) return 1; return 0;'

echo OK

