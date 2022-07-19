#ifndef _GPIB_BUS_H_
#define _GPIB_BUS_H_

#include <stdint.h>

#define DEVICE_COUNT 16

extern uint8_t(*gpib_cbRead[DEVICE_COUNT])(void* udata, uint32_t portnum);
extern uint16_t(*ports_cbReadW[PORTS_COUNT])(void* udata, uint32_t portnum);
extern void (*ports_cbWriteB[PORTS_COUNT])(void* udata, uint32_t portnum, uint8_t value);
extern void (*ports_cbWriteW[PORTS_COUNT])(void* udata, uint32_t portnum, uint16_t value);
extern void* ports_udata[PORTS_COUNT];

void gpib_cbRegister(uint32_t start, uint32_t count, uint8_t(*readb)(void*, uint32_t), uint16_t(*readw)(void*, uint32_t), void (*writeb)(void*, uint32_t, uint8_t), void (*writew)(void*, uint32_t, uint16_t), void* udata);
void gpib_init();

#endif
