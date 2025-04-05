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

#include <stdint.h>
#include <stdbool.h>
#include "gridvideo.h"
#include "config.h"
#include "timing.h"
#include "utility.h"
#include "memory.h"
#include "sdlconsole.h"
#include "debuglog.h"
#include "i8259.h"

// fixme: make two resolutions in runtime
uint32_t sdl_framebuffer[screenHeight][screenWidth];
//uint32_t gridvideo_framebuffer1101[240][320];
//uint32_t gridvideo_framebuffer1139[256][512];
uint8_t gridvideo_RAM[(screenWidth * screenHeight) / 8];

volatile uint8_t gridvideo_doDraw = 1;

int gridvideo_init() {

    memory_mapCallbackRegister(VIDEOBASE, (screenWidth * screenHeight) / 8, (void*)gridvideo_readmemory, (void*)gridvideo_writememory, NULL);

    //timing_addTimer(gridvideo_scanlineCallback, 16000, TIMING_ENABLED);
    //timing_addTimer(gridvideo_scanlineCallback, 100, TIMING_ENABLED);
    timing_addTimer(gridvideo_scanlineCallback, 200, TIMING_ENABLED);

    // do we need this?
    //timing_addTimer(gridvideo_drawCallback, NULL, 60, TIMING_ENABLED);

    return 0;
}

void gridvideo_update() {

    if (gridvideo_doDraw == 0) {
        return;     // no new data in videoRAM. nothing to do
    }
    
    gridvideo_doDraw = 0;

    int32_t scx, scy;
    int32_t order;
    
    for (int32_t addr = 0; addr < ( (screenWidth * screenHeight) / 8); addr++) {
        if ( (addr & 0x1) == true) {
            order = -8;
        } else {
            order = 8;
        }

        scy = addr / (screenWidth / 8);
        scx = addr - scy * (screenWidth / 8);
        for (int32_t i = 7; i >= 0; i--) {
            sdl_framebuffer[scy][scx * 8 + (7 - i) + order] = bitRead(gridvideo_RAM[addr], i) ? GRID_SCREEN_COLOR : 0;
        }
    }
    
    sdlconsole_blit((uint32_t *)sdl_framebuffer, screenWidth, screenHeight, screenWidth * sizeof(uint32_t));
}

void gridvideo_writememory(void* dummy, uint32_t addr, uint8_t value) {
    addr -= VIDEOBASE;
    if (addr >= ( (screenWidth * screenHeight) / 8) ) {
        return;
    }

    gridvideo_RAM[addr] = value;
    gridvideo_doDraw = 1;
}

uint8_t gridvideo_readmemory(void* dummy, uint32_t addr) {
    addr -= VIDEOBASE;
    if (addr >= ( (screenWidth * screenHeight) / 8) ) {
        return 0xFF;
    }
    return gridvideo_RAM[addr];
}

void gridvideo_scanlineCallback(void* dummy) {
    //i8259_doirq(irqLineSync);
    i8259_setirq(irqLineSync, true);
    gridvideo_doDraw = 1;
    //debug_log(DEBUG_DETAIL, "[video] setIRQ\n");
}

// do we need this?
//void gridvideo_drawCallback(void* dummy) {
//        gridvideo_doDraw = 1;
//}
