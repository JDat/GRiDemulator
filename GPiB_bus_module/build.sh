#!/bin/sh
rm gpibtest
#gcc -g -O0 -o bin/xtulator XTulator/*.c XTulator/chipset/*.c XTulator/cpu/cpu.c XTulator/modules/audio/*.c XTulator/modules/disk/*.c XTulator/modules/input/*.c XTulator/modules/io/*.c XTulator/modules/video/*.c -lm -lpthread `pcap-config --cflags --libs` `sdl2-config --cflags --libs`
#gcc -g -O0 -o bin/xtulator XTulator/*.c XTulator/chipset/*.c XTulator/cpu/cpu.c XTulator/modules/video/*.c -lm -lpthread `pcap-config --cflags --libs` `sdl2-config --cflags --libs`
#gcc -g -O0 -o bin/xtulator XTulator/*.c XTulator/chipset/*.c -lm -lpthread `pcap-config --cflags --libs` `sdl2-config --cflags --libs`
# gcc -Wall -g -O0 -o gpibtest *.c -lm -lpthread `pcap-config --cflags --libs` `sdl2-config --cflags --libs`
gcc -Wall -g -O0 -o gpibtest *.c -lm -lpthread
