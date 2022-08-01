#include <stdio.h>
#include <stdint.h>

#include <string.h>
#include "config.h"
#include "debuglog.h"
#include "machine.h"
#include "GPIB_bus.h"
#include "utility.h"

uint8_t GPIBbusStatus;
uint8_t GPIBbusData;

void GPIBbusIFCset(bool pin) {
        bitWrite(GPIBbusStatus, pinIFC, pin);
}

bool GPIBbusIFCget() {
        return bitRead(GPIBbusStatus, pinIFC);
}

void GPIBbusStatusSet(uint8_t status) {
        GPIBbusStatus = status;
}

uint8_t GPIBbusStatusGet() {
        return GPIBbusStatus;
}

void GPIBbusDataSet(uint8_t data) {
        GPIBbusData = data;
}

uint8_t GPIBbusDataGet() {
        return GPIBbusData;
}
