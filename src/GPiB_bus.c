/*
  GRiD Compass emulator
  Copyright (C)2022 JDat
  https://github.com/JDat/GRiDemulator

  This program is free software; you can redistribute it and/or
  modify it under the terms of the GNU General Public License
  as published by the Free Software Foundation; either version 2
  of the License, or (at your option) any later version.

  This program is distributed in the hope that it will be useful,
  but WITHOUT ANY WARRANTY; without even the implied warranty of
  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
  GNU General Public License for more details.

  You should have received a copy of the GNU General Public License
  along with this program; if not, write to the Free Software
  Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA.
*/

#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>

#include "config.h"
#include "debuglog.h"
//#include "machine.h"
#include "GPiB_bus.h"
#include "utility.h"

uint8_t GPiBbusStatus;
uint8_t GPiBbusData;

bool mutex = false;
//voidFuncPtr intFunc;

struct _client clients[DEVICE_COUNT];

void lockMutex() {
    while (mutex == true) {
        ;
    }
    mutex = true;
}

void freeMutex() {
    mutex = false;
}

uint8_t getStatusMutex() {
    lockMutex();
    freeMutex();
    return GPiBbusStatus;
}

uint8_t getDataMutex() {
    lockMutex();
    freeMutex();
    return GPiBbusData;
}

void notify(typeClientId clientId) {
    lockMutex();
    
    GPiBbusStatus = 0xff;
    GPiBbusData = 0xff;
    for (uint8_t i = 0; i < DEVICE_COUNT; i++) {
        if (clients[i].used == true) {
            GPiBbusStatus &= clients[i].status;
            GPiBbusData &= clients[i].data;
        }
    }

    freeMutex();
    
    for (uint8_t i = 0; i < DEVICE_COUNT; i++) {
        if (clients[i].used == true && clients[i].clientId != clientId) {
            clients[i].receiverFunctionCallBack();
        }
    }
}

void GPiBbusSRQwrite(bool pin, typeClientId clientId) {
    bitWrite(clients[clientId].status, pinSRQ, pin);
    notify(clientId);
}

bool GPiBbusSRQget() {
    (void)getStatusMutex();
    return bitRead(GPiBbusStatus, pinSRQ);
}

void GPiBbusATNwrite(bool pin, typeClientId clientId) {
    bitWrite(clients[clientId].status, pinATN, pin);
    notify(clientId);
}
bool GPiBbusATNget() {
    (void)getStatusMutex();
    return bitRead(GPiBbusStatus, pinATN);
}

void GPiBbusEOIwrite(bool pin, typeClientId clientId) {
    bitWrite(clients[clientId].status, pinEOI, pin);
    notify(clientId);
}
bool GPiBbusEOIget() {
    (void)getStatusMutex();
    return bitRead(GPiBbusStatus, pinEOI);
}

void GPiBbusDAVwrite(bool pin, typeClientId clientId) {
    bitWrite(clients[clientId].status, pinDAV, pin);
    notify(clientId);
}

bool GPiBbusDAVget() {
    (void)getStatusMutex();
    return bitRead(GPiBbusStatus, pinDAV);
}

void GPiBbusNRFDwrite(bool pin, typeClientId clientId) {
    bitWrite(clients[clientId].status, pinNRFD, pin);
    notify(clientId);
}
bool GPiBbusNRFDget() {
    (void)getStatusMutex();
    return bitRead(GPiBbusStatus, pinNRFD);
}

void GPiBbusNDACwrite(bool pin, typeClientId clientId) {
    bitWrite(clients[clientId].status, pinNDAC, pin);
    notify(clientId);
}

bool GPiBbusNDACget() {
    (void)getStatusMutex();
    return bitRead(GPiBbusStatus, pinNDAC);
}

void GPiBbusIFCwrite(bool pin, typeClientId clientId) {
    bitWrite(clients[clientId].status, pinIFC, pin);
    notify(clientId);
}

bool GPiBbusIFCget() {
    (void)getStatusMutex();
    return bitRead(GPiBbusStatus, pinIFC);
}

void GPiBbusRENwrite(bool pin, typeClientId clientId) {
    bitWrite(clients[clientId].status, pinREN, pin);
    notify(clientId);
}

bool GPiBbusRENget() {
    (void)getStatusMutex();
    return bitRead(GPiBbusStatus, pinREN);
}

void GPiBbusStatusWrite(uint8_t status, typeClientId clientId) {
    clients[clientId].status = status;
    notify(clientId);
}

uint8_t GPiBbusStatusGet() {
    return getStatusMutex();
}

void GPiBbusDataWrite(uint8_t data, typeClientId clientId) {
    clients[clientId].data = data;
    notify(clientId);
}

uint8_t GPiBbusDataGet() {
    return getDataMutex();
}


int8_t GRiBsearchFirstFreeClient() {
    for (uint8_t i = 0; i < DEVICE_COUNT; i++) {
        if (clients[i].used == false) {
            return i;
        }
    }
    return -1;
}

void GPiBinit() {
    lockMutex();
    
    GPiBbusStatus = 0xff;
    GPiBbusData = 0xff;
    
    for (uint8_t i = 0; i < DEVICE_COUNT; i++) {
        clients[i].clientId = i;
        clients[i].data = 0xff;
        memset(clients[i].description, 0x00, sizeof(clients[0].description));
        clients[i].receiverFunctionCallBack = 0;
        clients[i].status = 0xff;
        clients[i].used = false;
    }
    freeMutex();
}

int8_t GPiBregisterClient(void (*userFunc)(void), char desc[32]) {
    int8_t ret;
    lockMutex();
    
    ret = GRiBsearchFirstFreeClient();
    if (ret < 0) return -1;
    clients[ret].used = true;
    clients[ret].receiverFunctionCallBack = userFunc;
    clients[ret].clientId = (typeClientId)ret;
    strcpy(clients[ret].description, desc);
    freeMutex();
    return ret;
}

int8_t GPiBdumpClientsArray() {
    for (uint8_t i = 0; i < DEVICE_COUNT; i++) {
        printf("Num: %d\t", i);
        printf("Used: %d\t", clients[i].used);
        printf("ID: %d\t", clients[i].clientId);
        printf("Descr: %s\t", clients[i].description);
        printf("Data: 0x%02X\t", clients[i].data);
        printf("Status: 0x%02X\t", clients[i].status);
        printf("\n");
    }
    return 0;
}
