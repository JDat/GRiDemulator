#ifndef _MODEM_I8255_H_
#define _MODEM_I8255_H_

#include <stdint.h>
//#include "cpu.h"

uint8_t modem_read(void* dummy, uint32_t addr);
void modem_write(void* dummy, uint32_t addr, uint8_t data);

void modem_init();

#endif
