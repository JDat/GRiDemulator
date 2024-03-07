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
#include <stddef.h>
#include <pthread.h>
#include "gridvideo.h"
#include "config.h"
#include "timing.h"
#include "utility.h"
#include "memory.h"
#include "sdlconsole.h"
#include "debuglog.h"

pthread_t gridvideo_renderThreadID;
//uint32_t gridvideo_framebuffer[240][320];
uint32_t gridvideo_framebuffer1101[240][320];
uint32_t gridvideo_framebuffer1139[256][512];
//uint32_t gridvideo_framebuffer[256][512];
uint8_t *gridvideo_RAM = NULL;

uint32_t screenX, screenY;
volatile uint8_t gridvideo_doDraw = 1;

int gridvideo_init() {
        //int x, y;

#ifdef DEBUG_GRIDVIDEO
        debug_log(DEBUG_INFO, "[GRiD Video] Initializing GRiD Video device\r\n");
        debug_log(DEBUG_INFO, "[GRiD Video] Device num 0x%02X\r\n", videocard);
#endif

        // Fixme: force to 320x240 for now
        videocard = VIDEO_CARD_GRID1101;

        switch(videocard) {
                case VIDEO_CARD_GRID1101:
                        screenX = 320;
                        screenY = 240;
                        break;
                case VIDEO_CARD_GRID1139:
                        screenX = 512;
                        screenY = 256;
                        break;
                default:
#ifdef DEBUG_GRIDVIDEO
                        debug_log(DEBUG_INFO, "[GRiD Video] unknown device num 0x%02X\r\n", videocard);
#endif
                        return -1;
        }

#ifdef DEBUG_GRIDVIDEO
        debug_log(DEBUG_INFO, "[GRiD Video] Attaching to memory\r\n");
#endif

        memory_mapCallbackRegister(0x400, (screenX * screenY) / 8, (void*)gridvideo_readmemory, (void*)gridvideo_writememory, NULL);

// fixme:  don know hot resolution is switched  in 1139
        //screenX = 320;
        //screenY = 240;

        gridvideo_RAM = (uint8_t*)malloc( (screenX * screenY) / 8);

        if (gridvideo_RAM == NULL) {
#ifdef DEBUG_GRIDVIDEO
                debug_log(DEBUG_ERROR, "[GRiD Video] Failed to allocate video memory\r\n");
#endif
                return -1;
        }

#ifdef DEBUG_GRIDVIDEO
                        debug_log(DEBUG_INFO, "[GRiD Video] malloc complete\r\n");
#endif


        //timing_addTimer(gridvideo_scanlineCallback, NULL, 62800, TIMING_ENABLED);
        timing_addTimer(gridvideo_drawCallback, NULL, 60, TIMING_ENABLED);

        //TODO: error checking below
        pthread_create(&gridvideo_renderThreadID, NULL, gridvideo_renderThread, NULL);


        return 0;
}

void gridvideo_update() {
        uint32_t scx, scy;
        int8_t order;

        for (uint32_t addr = 0; addr < ( (screenX * screenY) / 8); addr++) {
                if (addr & 0x1) {
                        order = -8;
                } else {
                        order = 8;
                }

                scy = (uint32_t)(addr / (screenX / 8) );
                scx = (addr - scy * (screenX / 8) );
                for (int i = 7; i >= 0; i--) {

                        switch(videocard) {
                                case VIDEO_CARD_GRID1101:
                                        gridvideo_framebuffer1101[scy][scx * 8 + (7 - i) + order] = bitRead(gridvideo_RAM[addr], i) ? GRID_SCREEN_COLOR : 0;
                                        break;
                                case VIDEO_CARD_GRID1139:
                                        gridvideo_framebuffer1139[scy][scx * 8 + (7 - i) + order] = bitRead(gridvideo_RAM[addr], i) ? GRID_SCREEN_COLOR : 0;
                                        break;
                        }
                }
        }


        switch(videocard) {
                case VIDEO_CARD_GRID1101:
                        sdlconsole_blit((uint32_t *)gridvideo_framebuffer1101, screenX, screenY, screenX * sizeof(uint32_t));
                        break;
                case VIDEO_CARD_GRID1139:
                        sdlconsole_blit((uint32_t *)gridvideo_framebuffer1139, screenX, screenY, screenX * sizeof(uint32_t));
                        break;
        }
}

void *gridvideo_renderThread(void* dummy) {
        while (running) {
                if (gridvideo_doDraw == 1) {
                        gridvideo_update();
                        gridvideo_doDraw = 0;
                }
                else {
                        utility_sleep(1);
                }
        }
        pthread_exit(NULL);
}

void gridvideo_writememory(void* dummy, uint32_t addr, uint8_t value) {
        addr -= 0x400;
        if (addr >= ( (screenX  * screenY) / 8) ) return;
        gridvideo_RAM[addr] = value;
}

uint8_t gridvideo_readmemory(void* dummy, uint32_t addr) {
        addr -= 0x400;
        if (addr >= ( (screenX  * screenY) / 8) ) return 0xFF;
        return gridvideo_RAM[addr];
}

void gridvideo_scanlineCallback(void* dummy) {
        static uint16_t scanline = 0, hpart = 0;

        hpart++;
        if (hpart == 4) {
                /*if (scanline < 200) {
                        cga_update(0, (scanline<<1), 639, (scanline<<1)+1);
                }*/
                hpart = 0;
                scanline++;
        }
        if (scanline == screenY) {
                scanline = 0;
        }
}

void gridvideo_drawCallback(void* dummy) {
        gridvideo_doDraw = 1;
}
