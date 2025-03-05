#!/bin/sh

mkdir -p target
cc src/main.c -lncurses -lnwge_bndl -lSDL2 -O3 -std=c99 -otarget/nwgebndl-tui
