#ifndef _GPIB_TMS9914A_H_
#define _GPIB_TMS9914A_H_

#include <stdint.h>
//#include "cpu.h"

uint8_t tms9914a_read(void* dummy, uint32_t addr);
void tms9914a_write(void* dummy, uint32_t addr, uint8_t data);

void tms9914a_init();

#endif
