#ifndef _GPIB_BUS_H_
#define _GPIB_BUS_H_

#include <stdint.h>
#include <stdbool.h>

#define DEVICE_COUNT 16

enum {
    pinSRQ  = 0,        // slave
    pinATN  = 1,        // controller
    pinEOI  = 2,        // controller/slave
    pinDAV  = 3,        // controller
    pinNRFD = 4,        // slave
    pinNDAC = 5,        // slave
    pinIFC  = 6,        // controller
    pinREN  = 7         // controller
};

void GPIBbusIFCset(bool pin);
bool GPIBbusIFCget();

void GPIBbusStatusSet(uint8_t status);
uint8_t GPIBbusStatusGet();

void GPIBbusDataSet(uint8_t data);
uint8_t GPIBbusDataGet();

#endif
