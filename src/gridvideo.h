#ifndef _GRIDVIDEO_H_
#define _GRIDVIDEO_H_

#include <stdint.h>
#include "cpu.h"

#define GRID_SCREEN_COLOR    0xffeb00   // Amber color 0xffeb00 Sharp datasheet tells 585 nm wavelenght
int gridvideo_init();
//void gridvideo_update(uint32_t start_x, uint32_t start_y, uint32_t end_x, uint32_t end_y);

void gridvideo_update();
void gridvideo_scanlineCallback(void* dummy);
void *gridvideo_renderThread(void* cpu);
void gridvideo_writememory(void* dummy, uint32_t addr, uint8_t value);
uint8_t gridvideo_readmemory(void* dummy, uint32_t addr);
void gridvideo_drawCallback(void* dummy);

#endif
