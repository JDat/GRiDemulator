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
#include <stddef.h>
#include <pthread.h>
pthread_t gridvideo_renderThreadID;
#include "gridvideo.h"
#include "config.h"
#include "timing.h"
#include "utility.h"
//#include "ports.h"
#include "memory.h"
#include "sdlconsole.h"
#include "debuglog.h"

//uint8_t gridvideo_font[4096];
uint32_t gridvideo_framebuffer[240][320];
uint8_t *gridvideo_RAM = NULL;

volatile uint8_t gridvideo_doDraw = 1;

int gridvideo_init() {
	int x, y;

	debug_log(DEBUG_INFO, "[GRiD Video] Initializing GRid Video device\r\n");

	gridvideo_RAM = (uint8_t*)malloc(9600);
	if (gridvideo_RAM == NULL) {
		debug_log(DEBUG_ERROR, "[GRiD Video] Failed to allocate video memory\r\n");
		return -1;
	}
        utility_loadFile(gridvideo_RAM, 3320, "ROMS/screenLogo.bin");

	for (y = 0; y < 240; y++) {
		for (x = 0; x < 320; x++) {
			gridvideo_framebuffer[y][x] = 0;
		}
	}
	sdlconsole_blit((uint32_t *)gridvideo_framebuffer, 320, 240, 320 * sizeof(uint32_t));

	timing_addTimer(gridvideo_scanlineCallback, NULL, 62800, TIMING_ENABLED);
	timing_addTimer(gridvideo_drawCallback, NULL, 60, TIMING_ENABLED);
	/*
		NOTE: CGA scanlines are clocked at 15.7 KHz. We are breaking each scanline into
		four parts and using the last part as a very approximate horizontal retrace period.
		
		15700 x 4 = 62800

		See cga_scanlineCallback function for more details.
	*/

	//TODO: error checking below
#ifdef _WIN32
	_beginthread(gridvideo_renderThread, 0, NULL);
#else
	pthread_create(&gridvideo_renderThreadID, NULL, gridvideo_renderThread, NULL);
#endif
        debug_log(DEBUG_INFO, "[GRiD Video] Attaching to memory\r\n");
	memory_mapCallbackRegister(0x400, 0x02580, (void*)gridvideo_readmemory, (void*)gridvideo_writememory, NULL);

	return 0;
}

//void gridvideo_update(uint32_t start_x, uint32_t start_y, uint32_t end_x, uint32_t end_y) {
void gridvideo_update() {
	uint32_t scx, scy;
	int8_t order;

	//debug_log(DEBUG_DETAIL, "Update grid video device\r\n");
	for (uint32_t addr = 0; addr < (320 * 240 / 8); addr++) {
		if (addr & 0x1) {
			order = -8;
		} else {
			order = 8;
		}
		
		scy = (uint32_t)(addr / 40);
		scx = (addr - scy * 40);
		//printf("y: %d\n", scy);
		for (int i = 7; i >= 0; i--) {                
			gridvideo_framebuffer[scy][scx * 8 + (7 - i) + order] = bitRead(gridvideo_RAM[addr], i) ? GRID_SCREEN_COLOR : 0;
			//p = bitRead(gridvideo_RAM[addr], i) ? GRID_SCREEN_COLOR : 0;
			//printf("xb: %d, x: %d, i: %d, addr: %d\n", scx, scx * 8 + (7 - i) + order, i, addr);                        
		}
	}
	
	//sdlconsole_blit((uint32_t *)gridvideo_framebuffer, 320, 240, 240 * sizeof(uint32_t));
        sdlconsole_blit((uint32_t *)gridvideo_framebuffer, 320, 240, 320 * sizeof(uint32_t));
}

//void gridvideo_renderThread(void* dummy) {
void *gridvideo_renderThread(void* dummy) {
	while (running) {
		if (gridvideo_doDraw == 1) {
			//gridvideo_update(0, 0, 319, 239);
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
	//debug_log(DEBUG_DETAIL, "[GRiD Video] Writing to video buffer. Addr: 0x%05X\tdata; 0x%02X\r\n", addr, value);
	addr -= 0x400;
	if (addr >= 0x02580) return;
        //debug_log(DEBUG_INFO, "Good\r\n");
	gridvideo_RAM[addr] = value;
}

uint8_t gridvideo_readmemory(void* dummy, uint32_t addr) {
	//debug_log(DEBUG_DETAIL, "[GRiD Video] Reading from video buffer. Addr: 0x%05X\r\n", addr);
	addr -= 0x400;
	if (addr >= 0x02580) return 0xFF;
        //debug_log(DEBUG_INFO, "Good\r\n");
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
	if (scanline == 240) {
		scanline = 0;
	}
}

void gridvideo_drawCallback(void* dummy) {
	gridvideo_doDraw = 1;
}
