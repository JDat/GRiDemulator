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
#define STR_VERSION "0.0.0.1-JDat"

//#define DEBUG_TIMING

//#define DEBUG_CPU
//#define DEBUG_FPU
#define DEBUG_DISASM
#define DEBUG_MEMORY
//#define DEBUG_PORTS

//#define DEBUG_KEYBOARD
//#define DEBUG_GRIDVIDEO
//#define DEBUG_PIC
//#define DEBUG_RTC
//#define DEBUG_GPIB
//#define DEBUG_BUBBLEMEM
//#define DEBUG_UART
//#define DEBUG_MODEM

#define CPU_ADDRESS_RANGE		0x100000    // CPU address range
#define CPU_ADDRESS_MASK		0x0FFFFF    // mask in case something wrong and out of CPU addr range

//#define PORTS_COUNT 0xFFFF    // this is maximum for 8086
#define PORTS_COUNT 0x000F    // For GRiD only 0x00 and 0x02 used for 8259 IRQ controller
  
#define VIDEO_CARD_GRID_320x240 	1
#define VIDEO_CARD_GRID_512x256 	2

#define screenWidth 320
#define screenHeight 240
#define VIDEOBASE 0x400

//#define VIDEO_CARD_GRID1139 	2

#define GRID_SCREEN_COLOR    0xFFEB00   // Amber color 0xffeb00 Sharp datasheet tells 585 nm wavelenght
//#define GRID_SCREEN_COLOR    0xFFC000   // Amber color 0xffC000 By wikipedia and online color pickers

#define irqSerial   0
#define irqBubble   1
#define irqModem    2
#define irqLineSync 3
#define irqKeyboard 4
#define irqGPiB     5
#define irq8087     6
#define irqGlitch   7

// Interrupt Descriptor table
#define ramIDTbaseAddress 0
#define ramIDTSize 0x400

// Main RAM
#define ramMainBaseAddress (ramIDTSize + (screenWidth / 8 * screenHeight))
#define ramMainSize 0x40000 - ramMainBaseAddress
//#define ramMainSize 0x80000 - ramMainBaseAddress

#define DMABASE 0xE0000
#define DMALEN 0x0FFFF


#define SAMPLE_RATE		16000
//#define SAMPLE_BUFFER	4800

//#define _stricmp strcasecmp

extern volatile uint8_t running;
//extern uint8_t videocard, showMIPS;
extern double speedarg;
extern volatile double speed;
extern uint32_t baudrate, ramsize;
extern char* usemachine;
extern uint8_t bootdrive;

void setspeed(double mhz);

#endif
