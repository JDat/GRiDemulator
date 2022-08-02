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
	TMS914A GPIB controller
*/

#include <stdio.h>
#include <stdint.h>
#include <string.h>
#include <stdbool.h>
#include "config.h"
#include "modem.h"
#include "i8259.h"
#include "memory.h"
#include "debuglog.h"
#include "utility.h"

//#include <time.h>

#define baseAddress 0xDFEC0
#define addressLen  0x7

// Auxiliary commands

I8259_t* i8259;

uint8_t modem_read(void* dummy, uint32_t addr) {
        addr = addr - baseAddress;
        addr = addr >> 1;

#ifdef DEBUG_MODEM
        debug_log(DEBUG_DETAIL, "[modem] Read port 0x%02X\n", addr);
#endif
        return 0;

}


void modem_write(void* dummy, uint32_t addr, uint8_t value) {
        addr = addr - baseAddress;
        addr = addr >> 1;
#ifdef DEBUG_MODEM
            debug_log(DEBUG_DETAIL, "[modem] Write port: 0x%01X\tValue: 0x%02X\n", addr, value);
#endif
}


void modem_init() {
#ifdef DEBUG_MODEM
        debug_log(DEBUG_INFO, "[modem] Initializing modem\r\n");
#endif
        memory_mapCallbackRegister(baseAddress, addressLen, (void*)modem_read, (void*)modem_write, NULL);
}
