#ifndef _GPIB_BUS_H_
#define _GPIB_BUS_H_

#include <stdint.h>

#define DEVICE_COUNT 16

uint8_t GPIBbusStatus;
uint8_t GPIBbusData;

void GPIBbusStatusSet(uint8_t status);
uint8_t GPIBbusStatusGet();

void GPIBbusDataSet(uint8_t data);
uint8_t GPIBbusDataGet();

#endif
