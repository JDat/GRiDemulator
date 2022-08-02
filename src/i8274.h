#ifndef _UART_I8274_H_
#define _UART_I8274_H_

#include <stdint.h>
//#include "cpu.h"

uint8_t uart_read(void* dummy, uint32_t addr);
void uart_write(void* dummy, uint32_t addr, uint8_t data);

void uart_init();

#endif
