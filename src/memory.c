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


#include <stdio.h>
#include <stdint.h>
#include <stdbool.h>
#include <stdlib.h>
#include "config.h"
#include "cpu.h"
#include "utility.h"
#include "memory.h"
#include "debuglog.h"

#include "machine.h"

uint8_t* memory_mapRead[CPU_ADDRESS_RANGE];
uint8_t* memory_mapWrite[CPU_ADDRESS_RANGE];
uint8_t (*memory_mapReadCallback[CPU_ADDRESS_RANGE])(void* udata, uint32_t addr);
void (*memory_mapWriteCallback[CPU_ADDRESS_RANGE])(void* udata, uint32_t addr, uint8_t value);
void* memory_udata[CPU_ADDRESS_RANGE];

void cpu_write(CPU_t* cpu, uint32_t addr32, uint8_t value) {
    addr32 &= CPU_ADDRESS_MASK;
#ifdef DEBUG_MEMORY
        debug_log(DEBUG_INFO, "[MEM] cpu write Addr: 0x%05X, val: 0x%02X\n", addr32, value);
#endif

    if (memory_mapWrite[addr32] != NULL) {
        *(memory_mapWrite[addr32]) = value;
    } else if (memory_mapWriteCallback[addr32] != NULL) {
        (*memory_mapWriteCallback[addr32])(memory_udata[addr32], addr32, value);
    } else {
        debug_log(DEBUG_ERROR, "[MEM] Shit while CPU write. Addr: 0x%05X\t Value: 0x%02X\r\n", addr32, value);
    }
}

uint8_t cpu_read(CPU_t* cpu, uint32_t addr32) {
    addr32 &= CPU_ADDRESS_MASK;
#ifdef DEBUG_MEMORY
        debug_log(DEBUG_INFO, "[MEM] cpu read Addr: 0x%05X\t", addr32);
#endif
        
    if (memory_mapRead[addr32] != NULL) {
#ifdef DEBUG_MEMORY
        debug_log(DEBUG_INFO, "val: 0x%02X\n", *(memory_mapRead[addr32]));
#endif

        return *(memory_mapRead[addr32]);
    }

    if (memory_mapReadCallback[addr32] != NULL) {
#ifdef DEBUG_MEMORY
        debug_log(DEBUG_INFO, "val: 0x%02X\n", (*memory_mapReadCallback[addr32])(memory_udata[addr32], addr32));
#endif
        return (*memory_mapReadCallback[addr32])(memory_udata[addr32], addr32);
    }

        debug_log(DEBUG_ERROR, "[MEM] Shit while CPU read. Addr: 0x%05X\r\n", addr32);

    return 0xFF;
}

void memory_mapRegister(uint32_t start, uint32_t len, uint8_t* readb, uint8_t* writeb) {
#ifdef DEBUG_MEMORY
        debug_log(DEBUG_INFO, "[MEM] mapRegister start. Addr: 0x%05X, len: 0x%05X\r\n", start, len);
#endif
    uint32_t i;
    for (i = 0; i < len; i++) {
        if ((start + i) >= CPU_ADDRESS_RANGE) {
#ifdef DEBUG_MEMORY
                        debug_log(DEBUG_ERROR, "[MEM] Error in mapRegister: wanted: %d, allowed range: %d\r\n", (start + i), CPU_ADDRESS_RANGE);
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
        if ((start + i) >= CPU_ADDRESS_RANGE) {
#ifdef DEBUG_MEMORY
                        debug_log(DEBUG_ERROR, "[MEM] Error in mapCallbackRegister: wanted: %d, allowed range: %d\r\n", (start + i), CPU_ADDRESS_RANGE);
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

    for (i = 0; i < CPU_ADDRESS_RANGE; i++) {
        memory_mapRead[i] = NULL;
        memory_mapWrite[i] = NULL;
        memory_mapReadCallback[i] = NULL;
        memory_mapWriteCallback[i] = NULL;
        memory_udata[i] = NULL;
    }
    
    return 0;
}

void ramDump(uint32_t addr32, int32_t size) {
    int32_t i = size;
    
    while (i > 0) {
        for (uint32_t y = 0; y < 16; y++) {
            if (i > 0) debug_log(DEBUG_DETAIL, "%05X: ", addr32 + y * 16);
            for (uint32_t x = 0; x < 16; x++) {
                //debug_log(DEBUG_DETAIL, "[DEBUG: %05X]\n", addr32 + y * 16 + x);
                debug_log(DEBUG_DETAIL, "%02X ", (uint8_t) cpu_read(NULL, addr32 + y * 16 + x));
                i--;
                if (i <= 0) break;
            }
            if (i <= 0) break;
            debug_log(DEBUG_DETAIL, "\n");
        }
        addr32 += 256;
        debug_log(DEBUG_DETAIL, "\n");
    }
    debug_log(DEBUG_DETAIL, "\n");
}
