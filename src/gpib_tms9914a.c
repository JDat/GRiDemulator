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
#include "gpib_tms9914a.h"
#include "gpib_tms9914a_registers.h"
#include "memory.h"
#include "debuglog.h"
//#include <time.h>

#define baseAddress 0xDFF80
#define addressLen  0xF

// Auxiliary commands

void doAuxCmd(uint8_t value) {
        uint8_t bitFlag, command;
        bitFlag = (value  & 0b10000000) >> 7;
        command = (value & 0b00011111);
#ifdef DEBUG_GPIB
        debug_log(DEBUG_DETAIL, "[GPIB] Flag: %i\tAux cmd: 0x%02X, %s\n", bitFlag, auxCmd[command].cmd, auxCmd[command].name);
#endif

}
uint8_t tms9914a_read(void* dummy, uint32_t addr) {
        addr = addr - baseAddress;
        addr = addr >> 1;
#ifdef DEBUG_GPIB
        debug_log(DEBUG_DETAIL, "[GPIB] Read port 0x%02X\n", addr);
#endif
        return 0xFF;
}

void tms9914a_write(void* dummy, uint32_t addr, uint8_t value) {
        addr = addr - baseAddress;
        addr = addr >> 1;
        switch(addr) {
          case 3:
            doAuxCmd(value);
            break;
          default:
#ifdef DEBUG_GPIB
            debug_log(DEBUG_DETAIL, "[GPIB] Write port 0x%02X: %02X\n", addr, value);
#endif
            break;
        }
}

void tms9914a_init() {
#ifdef DEBUG_GPIB
        debug_log(DEBUG_INFO, "[GPIB] Initializing GPiB controller\r\n");
#endif
        memory_mapCallbackRegister(baseAddress, addressLen, (void*)tms9914a_read, (void*)tms9914a_write, NULL);
}
