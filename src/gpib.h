#ifndef _GPIB_H_
#define _GPIB_H_

#include <stdint.h>
//#include "cpu.h"

uint8_t gpib_read(void* dummy, uint32_t addr);
void gpib_write(void* dummy, uint32_t addr, uint8_t value);

void gpib_init();

#endif
