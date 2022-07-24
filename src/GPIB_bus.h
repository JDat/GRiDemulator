#ifndef _GPIB_BUS_H_
#define _GPIB_BUS_H_

#include <stdint.h>
#include <stdbool.h>

#define DEVICE_COUNT 16

enum {
    pinSRQ  = 0,
    pinATN  = 1,
    pinEOI  = 2,
    pinDAV  = 3,
    pinNRFD = 4,
    pinNDAC = 5,
    pinIFC  = 6,
    pinREN  = 7
};

void GPIBbusIFCset(bool pin);
bool GPIBbusIFCget();

void GPIBbusStatusSet(uint8_t status);
uint8_t GPIBbusStatusGet();

void GPIBbusDataSet(uint8_t data);
uint8_t GPIBbusDataGet();

#endif
