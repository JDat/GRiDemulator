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

#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>
#include "config.h"
#include "cpu.h"
#include "utility.h"
#include "memory.h"
#include "debuglog.h"

uint8_t* memory_mapRead[MEMORY_RANGE];
uint8_t* memory_mapWrite[MEMORY_RANGE];
uint8_t (*memory_mapReadCallback[MEMORY_RANGE])(void* udata, uint32_t addr);
void (*memory_mapWriteCallback[MEMORY_RANGE])(void* udata, uint32_t addr, uint8_t value);
void* memory_udata[MEMORY_RANGE];

void cpu_write(CPU_t* cpu, uint32_t addr32, uint8_t value) {
	addr32 &= MEMORY_MASK;

        //debug_log(DEBUG_INFO, "[MEM] cpu write. Addr: 0x%05X, val: 0x%02X\r\n", addr32, value);
	if (memory_mapWrite[addr32] != NULL) {
		*(memory_mapWrite[addr32]) = value;
	}
	else if (memory_mapWriteCallback[addr32] != NULL) {
		(*memory_mapWriteCallback[addr32])(memory_udata[addr32], addr32, value);
	} else {
                debug_log(DEBUG_ERROR, "[MEM] Shit while CPU write. Addr: 0x%05X\t Value: 0x%02X\r\n", addr32, value);
        }
}

uint8_t cpu_read(CPU_t* cpu, uint32_t addr32) {
	addr32 &= MEMORY_MASK;

        //debug_log(DEBUG_INFO, "[MEM] cpu read. Addr: 0x%05X\r\n", addr32);
	if (memory_mapRead[addr32] != NULL) {
		return *(memory_mapRead[addr32]);
	}

	if (memory_mapReadCallback[addr32] != NULL) {
		return (*memory_mapReadCallback[addr32])(memory_udata[addr32], addr32);
	}
#ifdef DEBUG_MEMORY
        debug_log(DEBUG_ERROR, "[MEM] Shit while CPU read. Addr: 0x%05X\r\n", addr32);
#endif
	return 0xFF;
}

void memory_mapRegister(uint32_t start, uint32_t len, uint8_t* readb, uint8_t* writeb) {
#ifdef DEBUG_MEMORY
        debug_log(DEBUG_INFO, "[MEM] mapRegister start. Addr: 0x%05X, len: 0x%05X\r\n", start, len);
#endif
	uint32_t i;
	for (i = 0; i < len; i++) {
		if ((start + i) >= MEMORY_RANGE) {
#ifdef DEBUG_MEMORY
                        debug_log(DEBUG_ERROR, "[MEM] Error in mapRegister: wanted: %d, allowed range: %d\r\n", (start + i), MEMORY_RANGE);
#endif
			break;
		}
		memory_mapRead[start + i] = (readb == NULL) ? NULL : readb + i;
		memory_mapWrite[start + i] = (writeb == NULL) ? NULL : writeb + i;
	}
}

void memory_mapCallbackRegister(uint32_t start, uint32_t count, uint8_t(*readb)(void*, uint32_t), void (*writeb)(void*, uint32_t, uint8_t), void* udata) {
#ifdef DEBUG_MEMORY
	debug_log(DEBUG_INFO, "[MEM] mapCallbackRegister. Addr: 0x%05X, len: 0x%05X\r\n", start, count);
#endif
        uint32_t i;
	for (i = 0; i < count; i++) {
		if ((start + i) >= MEMORY_RANGE) {
#ifdef DEBUG_MEMORY
                        debug_log(DEBUG_ERROR, "[MEM] Error in mapCallbackRegister: wanted: %d, allowed range: %d\r\n", (start + i), MEMORY_RANGE);
#endif
			break;
		}
		memory_mapReadCallback[start + i] = readb;
		memory_mapWriteCallback[start + i] = writeb;
		memory_udata[start + i] = udata;
	}
}

int memory_init() {
	uint32_t i;

	for (i = 0; i < MEMORY_RANGE; i++) {
		memory_mapRead[i] = NULL;
		memory_mapWrite[i] = NULL;
		memory_mapReadCallback[i] = NULL;
		memory_mapWriteCallback[i] = NULL;
		memory_udata[i] = NULL;
	}

	return 0;
}
