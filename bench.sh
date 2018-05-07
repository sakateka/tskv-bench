#!/bin/bash

sum(){
    summa=$(sed -n '2,$s/ /=/p;1w/dev/stderr'|sort|tr '\n' ' '|\
            { fold -s -w80; echo > /dev/stderr; }|\
            tee /dev/stderr|sort -nk2|md5sum|\
            sed -r 's/([0-9])/\x1b[32m\1\x1b[0m/g;
                    s/([a-c])/\x1b[33m\1\x1b[0m/g;
                    s/([c-f])/\x1b[34m\1\x1b[0m/g')
    printf "%s\n" "$summa"
}
fmt="[95mt=%e m=%M[0m"

echo
printf "\x1b[36;1m   >>> C:  \x1b[0m"
clang -O3 -o tskv-c tskv-c.c -lpcre2-8 && { /usr/bin/time -f "$fmt" ./tskv-c; }|sum

echo
printf "\x1b[36;1m   >>> rust:  \x1b[0m"
cd tskv-rs; cargo build --release; ret=$?; cd - &>/dev/null
((ret == 0))&&{ /usr/bin/time -f "$fmt" tskv-rs/target/release/tskv-rs; }|sum

echo
printf "\x1b[36;1m   >>> go:  \x1b[0m"
go build tskv-go.go && { /usr/bin/time -f "$fmt" ./tskv-go; }|sum

echo
printf "\x1b[36;1m   >>> awk:  \x1b[0m"
{ /usr/bin/time -f "$fmt" ./tskv-awk.awk access.log; }|sum

echo
printf "\x1b[36;1m   >>> c++:  \x1b[0m"
clang++ -std=c++11 -O3 -o tskv-cc tskv-cc.cc && { /usr/bin/time -f "$fmt" ./tskv-cc; }|sum
