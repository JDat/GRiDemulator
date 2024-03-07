#!/bin/sh
rm gridemu
#gcc -Wall -g -O0 -o gridemu src/*.c -lm -lpthread `pcap-config --cflags --libs` `sdl2-config --cflags --libs`
gcc -Wall -g -O0 -o gridemu src/*.c -lm -lpthread `sdl2-config --cflags --libs`
