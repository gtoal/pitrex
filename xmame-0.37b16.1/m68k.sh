#!/bin/sh
OBJ=xmame.obj
mkdir -p ${OBJ}/cpu/m68000
gcc -DDOS -o m68kmake.exe src/cpu/m68000/m68kmake.c
cp m68kmake.exe ${OBJ}/cpu/m68000
${OBJ}/cpu/m68000/m68kmake.exe src/cpu/m68000 src/cpu/m68000/m68k_in.c
${OBJ}/cpu/m68000/m68kmake.exe ${OBJ}/cpu/m68000 src/cpu/m68000/m68k_in.c
