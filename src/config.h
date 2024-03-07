/*
  GRiD Compass emulator
  Copyright (C)2022 JDat
  https://github.com/JDat/GRiDemulator

  Based on XTulator: A portable, open-source 80186 PC emulator.
  Copyright (C)2020 Mike Chambers
  https://github.com/mikechambers84/XTulator

  This program is free software; you can redistribute it and/or
  modify it under the terms of the GNU General Public License
  as published by the Free Software Foundation; either version 2
  of the License, or (at your option) any later version.

  This program is distributed in the hope that it will be useful,
  but WITHOUT ANY WARRANTY; without even the implied warranty of
  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
  GNU General Public License for more details.

  You should have received a copy of the GNU General Public License
  along with this program; if not, write to the Free Software
  Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA.
*/

#ifndef _CONFIG_H_
#define _CONFIG_H_

#include <stdint.h>

#define STR_TITLE "GRID Compass Emulator"
#define STR_VERSION "0.20.7.15-JDat"

//#define DEBUG_TIMING

//#define DEBUG_CPU
//#define DEBUG_FPU
//#define DEBUG_DIASASM
//#define DEBUG_MEMORY
//#define DEBUG_PORTS

#define DEBUG_KEYBOARD
//#define DEBUG_GRIDVIDEO
//#define DEBUG_PIC
//#define DEBUG_PIT
//#define DEBUG_RTC
//#define DEBUG_GPIB
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
