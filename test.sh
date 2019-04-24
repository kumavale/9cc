#!/bin/bash

try() {
    expected="$1"
    input="$2"

    ./9cc "$input" > tmp.s
    gcc -o tmp tmp.s
    ./tmp
    actual="$?"

    if [ "$actual" = "$expected" ]; then
        echo "$input => $actual"
    else
        echo "$expected expected, but got $actual"
        exit 1
    fi
}

try 42 "42;"
try 21 '5+20-4;'
try 41 " 12 + 34 - 5 ;"
try 47 "5+6*7;"
try 15 "5*(9-6);"
try 4 "(3+5)/2;"
try 2 "1+2*(3-4);(4+2)/3;"
try 3 "a=1; a+a+a;"
try 6 "a=1; b=2; c=3; a*b*c;"
try 14 "a=3; b=5*6-8; a+b/2;"
try 0 "return 0;"
try 42 "r=42; return r;"
try 4 "a=b=2; a*b;"
try 6 "foo = 1; bar = 2 + 3; return foo + bar;"
try 13 "hoge=5; fuga=hoge*2; fuga+3;"

echo OK
