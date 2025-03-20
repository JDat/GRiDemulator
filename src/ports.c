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
#include <string.h>
#include "config.h"
#include "debuglog.h"
#include "cpu.h"
#include "machine.h"
#include "ports.h"

uint8_t (*ports_cbReadB[PORTS_COUNT])(uint32_t portnum);
//uint16_t (*ports_cbReadW[PORTS_COUNT])(uint32_t portnum);

void (*ports_cbWriteB[PORTS_COUNT])(uint32_t portnum, uint8_t value);
//void (*ports_cbWriteW[PORTS_COUNT])(uint32_t portnum, uint16_t value);

extern MACHINE_t machine;

void port_write(CPU_t* cpu, uint16_t portnum, uint8_t value) {
#ifdef DEBUG_PORTS
    debug_log(DEBUG_DETAIL, "[PORT] port_write @ %03X <- %02X\r\n", portnum, value);
#endif
    portnum &= 0x0FFF;
    if (ports_cbWriteB[portnum] != NULL) {
        (*ports_cbWriteB[portnum])(portnum, value);
        return;
    }
#ifdef DEBUG_PORTS
        debug_log(DEBUG_DETAIL, "[SHIT] not registered port_write @ %03X, data: %02X\r\n", portnum, value);
#endif
}

void port_writew(CPU_t* cpu, uint16_t portnum, uint16_t value) {
    portnum &= 0x0FFF;
    //if (ports_cbWriteW[portnum] != NULL) {
        //(*ports_cbWriteW[portnum])(portnum, value);
        //return;
    //}
    port_write(cpu, portnum, (uint8_t)value);
    port_write(cpu, portnum + 1, (uint8_t)(value >> 8));
}

uint8_t port_read(CPU_t* cpu, uint16_t portnum) {
#ifdef DEBUG_PORTS
    debug_log(DEBUG_DETAIL, "[PORT] port_read @ %03X\r\n", portnum);
#endif
    portnum &= 0x0FFF;
    if (ports_cbReadB[portnum] != NULL) {
        return (*ports_cbReadB[portnum])(portnum);
    }
#ifdef DEBUG_PORTS
        debug_log(DEBUG_DETAIL, "[SHIT] not registered port_read @ %03X\r\n", portnum);
#endif
    return 0xFF;
}

uint16_t port_readw(CPU_t* cpu, uint16_t portnum) {
    uint16_t ret;
    portnum &= 0x0FFF;
    //if (ports_cbReadW[portnum] != NULL) {
        //return (*ports_cbReadW[portnum])(portnum);
    //}
    ret = port_read(cpu, portnum);
    ret |= (uint16_t)port_read(cpu, portnum + 1) << 8;
    return ret;
}

//void ports_cbRegister(uint32_t start, uint32_t count, uint8_t (*readb)(uint32_t), uint16_t (*readw)(uint32_t), void (*writeb)(uint32_t, uint8_t), void (*writew)(uint32_t, uint16_t)) {
void ports_cbRegister(uint32_t start, uint32_t count, uint8_t (*readb)(uint32_t), void (*writeb)(uint32_t, uint8_t)) {
    
    uint32_t i;
    for (i = 0; i < count; i++) {
        if ((start + i) >= PORTS_COUNT) {
            break;
        }
        ports_cbReadB[start + i] = readb;
        //ports_cbReadW[start + i] = readw;
        ports_cbWriteB[start + i] = writeb;
        //ports_cbWriteW[start + i] = writew;
    }
}

void ports_init() {
    uint32_t i;
    for (i = 0; i < PORTS_COUNT; i++) {
        ports_cbReadB[i] = NULL;
        //ports_cbReadW[i] = NULL;
        ports_cbWriteB[i] = NULL;
        //ports_cbWriteW[i] = NULL;
    }
}
