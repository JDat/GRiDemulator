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


#include <sys/time.h>
#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>
#include "config.h"
#include "timing.h"
#include "debuglog.h"

uint64_t timing_cur;
uint64_t timing_freq;
TIMER* timers = NULL;
uint32_t timers_count = 0;

int timing_init() {
	timing_freq = 1000000;
	return 0;
}

void timing_loop() {
	uint32_t i;
	struct timeval tv;
	gettimeofday(&tv, NULL);
	timing_cur = (uint64_t)tv.tv_sec * 1000000 + (uint64_t)tv.tv_usec;

	for (i = 0; i < timers_count; i++) {
		if (timing_cur >= (timers[i].previous + timers[i].interval)) {
			if (timers[i].enabled != TIMING_DISABLED) {
				if (timers[i].callback != NULL) {
					(*timers[i].callback)(timers[i].data);
				}
				timers[i].previous += timers[i].interval;
				if ((timing_cur - timers[i].previous) >= (timers[i].interval * 100)) {
					timers[i].previous = timing_cur;
				}
			}
		}
	}
}

uint32_t timing_addTimerUsingInterval(void* callback, void* data, uint64_t interval, uint8_t enabled) {
	TIMER* temp;
	uint32_t ret;

	struct timeval tv;
	gettimeofday(&tv, NULL);
	timing_cur = (uint64_t)tv.tv_sec * 1000000 + (uint64_t)tv.tv_usec;

	temp = (TIMER*)realloc(timers, (size_t)sizeof(TIMER) * (timers_count + 1));
	if (temp == NULL) {
		//TODO: error handling
		return TIMING_ERROR; //NULL;
	}
	timers = temp;

	timers[timers_count].previous = timing_cur;
	timers[timers_count].interval = interval;
	timers[timers_count].callback = callback;
	timers[timers_count].data = data;
	timers[timers_count].enabled = enabled;

	ret = timers_count;
	timers_count++;

	return ret;
}

uint32_t timing_addTimer(void* callback, void* data, double frequency, uint8_t enabled) {
	return timing_addTimerUsingInterval(callback, data, (uint64_t)((double)timing_freq / frequency), enabled);
}

void timing_updateInterval(uint32_t tnum, uint64_t interval) {
	if (tnum >= timers_count) {
		debug_log(DEBUG_ERROR, "[ERROR] timing_updateInterval() asked to operate on invalid timer\r\n");
		return;
	}
	timers[tnum].interval = interval;
}

void timing_updateIntervalFreq(uint32_t tnum, double frequency) {
	if (tnum >= timers_count) {
		debug_log(DEBUG_ERROR, "[ERROR] timing_updateIntervalFreq() asked to operate on invalid timer\r\n");
		return;
	}
	timers[tnum].interval = (uint64_t)((double)timing_freq / frequency);
}

void timing_timerEnable(uint32_t tnum) {
	if (tnum >= timers_count) {
		debug_log(DEBUG_ERROR, "[ERROR] timing_timerEnable() asked to operate on invalid timer\r\n");
		return;
	}
	timers[tnum].enabled = TIMING_ENABLED;
	timers[tnum].previous = timing_getCur();
}

void timing_timerDisable(uint32_t tnum) {
	if (tnum >= timers_count) {
		debug_log(DEBUG_ERROR, "[ERROR] timing_timerDisable() asked to operate on invalid timer\r\n");
		return;
	}
	timers[tnum].enabled = TIMING_DISABLED;
}

uint64_t timing_getFreq() {
	return timing_freq;
}

uint64_t timing_getCur() {
	struct timeval tv;
	gettimeofday(&tv, NULL);
	timing_cur = (uint64_t)tv.tv_sec * 1000000 + (uint64_t)tv.tv_usec;

	return timing_cur;
}
