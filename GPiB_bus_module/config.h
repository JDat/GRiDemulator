#ifndef _CONFIG_H_
#define _CONFIG_H_

#include <stdint.h>

#define STR_TITLE "GRID Compass Emulator"
#define STR_VERSION "0.20.7.15-JDat"

//#define DEBUG_TIMING

#define DEBUG_CPU
//#define DEBUG_FPU
//#define DEBUG_DIASASM
//#define DEBUG_MEMORY
//#define DEBUG_PORTS

//#define DEBUG_KEYBOARD
//#define DEBUG_GRIDVIDEO
//#define DEBUG_PIC
//#define DEBUG_PIT
//#define DEBUG_RTC
#define DEBUG_GPIB
//#define DEBUG_BUBBLEMEM
//#define DEBUG_UART
//#define DEBUG_MODEM

#define VIDEO_CARD_GRID1101 	1
#define VIDEO_CARD_GRID1139 	2

#define SAMPLE_RATE		10000
//#define SAMPLE_BUFFER	4800

//#define _stricmp strcasecmp

extern volatile uint8_t running;
extern uint8_t videocard, showMIPS;
extern double speedarg;
extern volatile double speed;
extern uint32_t baudrate, ramsize;
extern char* usemachine;
extern uint8_t bootdrive;

void setspeed(double mhz);

#endif
