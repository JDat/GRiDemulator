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

#ifndef _DEBUGLOG_H_
#define _DEBUGLOG_H_

#include <stdint.h>

/*
	Debug levels:

	0 - No logging
	1 - Errors
	2 - Errors, info
	3 - Errors, info, detailed debugging
*/

#define DEBUG_NONE		0
#define DEBUG_ERROR		1
#define DEBUG_INFO		2
#define DEBUG_DETAIL	3

void debug_log(uint8_t level, char* format, ...);
void debug_setLevel(uint8_t level);
void debug_init();

#endif
