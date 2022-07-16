/*
  XTulator: A portable, open-source 80186 PC emulator.
  Copyright (C)2020 Mike Chambers

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

/*
	Bubble memory
*/

#include <stdio.h>
#include <stdint.h>
#include <string.h>
#include "config.h"
#include "i7220.h"
//#include "ports.h"
#include "memory.h"
#include "debuglog.h"
//#include <time.h>

#define baseAddress 0xDFE80
#define addressLen  0x3

uint8_t bubble_read(void* dummy, uint32_t addr) {
        addr = addr - baseAddress;
        debug_log(DEBUG_DETAIL, "[7220] Read port 0x%02X\n", addr);

        return 0x00;
}

void bubble_write(void* dummy, uint32_t addr, uint8_t value) {
        addr = addr - baseAddress;
        debug_log(DEBUG_DETAIL, "[7220] Write port 0x%02X: %02X\n", addr, value);
}

void bubble_init() {
	debug_log(DEBUG_INFO, "[7220] Initializing bubble memory controller\r\n");
        memory_mapCallbackRegister(baseAddress, addressLen, (void*)bubble_read, (void*)bubble_write, NULL);
}
