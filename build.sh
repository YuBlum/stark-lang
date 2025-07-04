#! /usr/bin/env sh
set -e
fasm ./starc-src/helper.s
gcc -Wall -Wextra -Werror -Wno-builtin-declaration-mismatch -fno-stack-protector -pedantic -std=c89 -nostdlib starc-src/starc.c starc-src/helper.o -o starc -ggdb
