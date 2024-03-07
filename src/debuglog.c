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


#include "config.h"
#include <stdio.h>
#include <stdint.h>
#include <stdarg.h>
#include "debuglog.h"
#include <time.h>

uint8_t debug_level = DEBUG_INFO;

void debug_log(uint8_t level, char* format, ...) {
	va_list argptr;
	va_start(argptr, format);
	if (level > debug_level) {
		va_end(argptr);
		return;
	}
	vfprintf(stderr, format, argptr);
  //vfprintf(stdout, format, argptr);
	fflush(stderr);
	va_end(argptr);
}

void debug_setLevel(uint8_t level) {
	if (level > DEBUG_DETAIL) {
		return;
	}
	debug_level = level;
}

void debug_init() {
	//TODO: Maybe allow initializing this with file output rather than always using stderr. Or maybe remove this, I don't know...
}
