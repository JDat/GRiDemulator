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
#include "i7220.h"

#include "machine.h"


#define DMABASE 0xE0000
#define DMALEN 0x0FFFF

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

                //debug_log(DEBUG_DETAIL, "[cpu] exec: Addr: %04X:%04X, opcode: %02X\r\n", machine.CPU.segregs[regcs], machine.CPU.ip, machine.CPU.opcode);
                //debug_log(DEBUG_DETAIL, "[cpu] regs: AX: %04X, BX: %04X, CX: %04X, DX: %04X\r\n", machine.CPU.regs.wordregs[regax], machine.CPU.regs.wordregs[regbx], machine.CPU.regs.wordregs[regcx], machine.CPU.regs.wordregs[regdx]);
                //debug_log(DEBUG_DETAIL, "[cpu] regs: SI: %04X, DI: %04X, BP: %04X, SP: %04X\r\n", machine.CPU.regs.wordregs[regsi], machine.CPU.regs.wordregs[regdi], machine.CPU.regs.wordregs[regbp], machine.CPU.regs.wordregs[regsp]);
                //debug_log(DEBUG_DETAIL, "[cpu] regs: CS: %04X, DS: %04X, ES: %04X, SS: %04X\r\n", machine.CPU.segregs[regcs], machine.CPU.segregs[regds], machine.CPU.segregs[reges], machine.CPU.segregs[regss]);

        }
}

uint8_t cpu_read(CPU_t* cpu, uint32_t addr32) {
	addr32 &= MEMORY_MASK;

        //if ( (addr32 >= 0xC0000) && (addr32 <= 0xCFFFF) ) {
                //debug_log(DEBUG_INFO, "[MEM] cpu read. Addr: 0x%05X\tval: 0x%02X\r\n", addr32, (uint8_t) *memory_mapRead[addr32] );
                //debug_log(DEBUG_INFO, "[MEM] cpu read. Addr: 0x%05X\r\n", addr32);
        //}
        
	if (memory_mapRead[addr32] != NULL) {
		return *(memory_mapRead[addr32]);
	}

	if (memory_mapReadCallback[addr32] != NULL) {
		return (*memory_mapReadCallback[addr32])(memory_udata[addr32], addr32);
	}
//#ifdef DEBUG_MEMORY
        debug_log(DEBUG_ERROR, "[MEM] Shit while CPU read. Addr: 0x%05X\r\n", addr32);

        //debug_log(DEBUG_DETAIL, "[cpu] exec: Addr: %04X:%04X, opcode: %02X\r\n", machine.CPU.segregs[regcs], machine.CPU.ip, machine.CPU.opcode);
        //debug_log(DEBUG_DETAIL, "[cpu] regs: AX: %04X, BX: %04X, CX: %04X, DX: %04X\r\n", machine.CPU.regs.wordregs[regax], machine.CPU.regs.wordregs[regbx], machine.CPU.regs.wordregs[regcx], machine.CPU.regs.wordregs[regdx]);
        //debug_log(DEBUG_DETAIL, "[cpu] regs: SI: %04X, DI: %04X, BP: %04X, SP: %04X\r\n", machine.CPU.regs.wordregs[regsi], machine.CPU.regs.wordregs[regdi], machine.CPU.regs.wordregs[regbp], machine.CPU.regs.wordregs[regsp]);
        //debug_log(DEBUG_DETAIL, "[cpu] regs: CS: %04X, DS: %04X, ES: %04X, SS: %04X\r\n", machine.CPU.segregs[regcs], machine.CPU.segregs[regds], machine.CPU.segregs[reges], machine.CPU.segregs[regss]);

//#endif
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
                //memory_mapRead[start + i] = 0;
		memory_mapWrite[start + i] = (writeb == NULL) ? NULL : writeb + i;
                //memory_mapWrite[start + i] = 0;
                if (start< 0xA0000) {
                        cpu_write(NULL,start+i,0xff);
                }
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

bool dmaActive;
uint16_t  dmaCount;

void doDMA() {
        //debug_log(DEBUG_ERROR, "[DMA] doDMA\n");
        while (dmaActive) {
                dmaCount++;
                if (dmaCount > 256) {
                        dmaActive = false;
                }
                dmaRead(DMABASE);
        }
}

//void dmaBubbleRequest() {
//        dmaActive  = true;
//        dmaRead(DMABASE);
//}

FILE *dmaFile;

int dmaInit() {
        if (dmaFile != NULL) {
                fclose(dmaFile);
        }
        dmaFile = fopen("ROMS/merged.raw", "rb");
        if (dmaFile == NULL) {

                debug_log(DEBUG_INFO, "[DMA] Error openimg DMA image\r\n");

        return -1;
        }
    
        memory_mapCallbackRegister(DMABASE, DMALEN, (void*)dmaRead, (void*)dmaWrite, NULL);
        return 0;
}


uint8_t dmaRead(uint32_t addr) {
        //size_t ret;
        uint8_t val;
        
        //val = bubble_read(NULL, 0xDFE80);
        //debug_log(DEBUG_ERROR, "[DMA] Read. Addr: %05X, Value: %02X\r\n", addr, val);
        //ret = fread(&val, 1, 1, dmaFile);
        (void)fread(&val, 1, 1, dmaFile);
        return val;
        
}

void dmaWrite(uint32_t addr, uint8_t value) {
        //debug_log(DEBUG_ERROR, "[DMA] Write. Addr: %05X\tValue: %02X\r\n", addr, value);
        //bubble_write(NULL, 0xDFE80, value);
}

void ramDump(uint32_t addr32) {
    for (uint32_t y = 0; y < 8; y++) {
        debug_log(DEBUG_DETAIL, "%05X: ", addr32 + y * 16);
        for (uint32_t x = 0; x < 16; x++) {
            debug_log(DEBUG_DETAIL, "%02X ", (uint8_t) *memory_mapRead[addr32 + y * 16 + x]);
        }
        debug_log(DEBUG_DETAIL, "\n");
    }
    debug_log(DEBUG_DETAIL, "\n");
}
