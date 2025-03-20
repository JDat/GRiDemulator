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

#ifndef _TIMING_H_
#define _TIMING_H_

#include <stdint.h>

typedef struct TIMER_s {
	uint64_t interval;
	uint64_t previous;
	uint8_t enabled;
	//void (*callback)(void*);
	void (*callback)();
	//void* data;
} TIMER;

#define TIMING_ENABLED	1
#define TIMING_DISABLED	0
#define TIMING_ERROR 0xFFFFFFFF

//#define TIMING_RINGSIZE	1024

int timing_init();
void timing_loop();
//uint32_t timing_addTimer(void* callback, void* data, double frequency, uint8_t enabled);
uint32_t timing_addTimer(void* callback, double frequency, uint8_t enabled);
void timing_updateIntervalFreq(uint32_t tnum, double frequency);
void timing_updateInterval(uint32_t tnum, uint64_t interval);
void timing_timerEnable(uint32_t tnum);
void timing_timerDisable(uint32_t tnum);
uint64_t timing_getFreq();
uint64_t timing_getCur();

extern uint64_t timing_cur;
extern uint64_t timing_freq;

#endif
