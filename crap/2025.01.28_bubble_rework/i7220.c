/*
  GRiD Compass emulator
  Copyright (C)2022 JDat
  https://github.com/JDat/GRiDemulator

  Based on MAMEdev project
  license:BSD-3-Clause
  copyright-holders:Sergey Svishchev
  https://github.com/mamedev/mame/blob/master/src/devices/machine/i7220.cpp

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

/*
	Bubble memory module emulator
*/

#include <stdio.h>
#include <stdint.h>
#include <stdbool.h>
#include <stdlib.h>
#include <string.h>
//#include <pthread.h>
#include <time.h>
#include "config.h"
#include "i7220.h"
#include "i7220_debug.h"
#include "i7220_private.h"
#include "i8259.h"
#include "memory.h"
#include "utility.h"
#include "debuglog.h"

#define delayTime 1

#define baseAddress 0xDFE80
#define addressLen  0x4

#define BUBBLEMODULECOUNT 3

//#define bubbleBufferSize 32
#define bubbleBufferSize 1
//#define BUBBLEFIFOSIZE 32


#define I7110_MBM_SIZE (128 * 1024) // 1 megabit
#define I7115_MBM_SIZE (512 * 1024) // 4 megabit

void commandStart (uint8_t cmd);
void commandEnd(uint8_t sucess);
void regUpdate();

void commandReadBubbleData();
void sectorRead();
void update_drq(I8259_t* i8259);

//pthread_t bubble_ThreadID;
FILE *bubbleFile;
I8259_t *irq8259;

//I8259_t* irq8259;

volatile uint8_t regRAC;
volatile uint8_t regArray[16];
volatile uint8_t regStatus;

volatile uint8_t bubbleCommand;
volatile uint8_t lastCommand;

int regBlockLen;
int regAddress;
int regBlockLenCount;
int regBlockLen_nfc;
int regAddress_addr;
int regAddress_mbm;
int regBubbleLimit;
int pageCounter = 0; // 256-bit pages
int fifoCounter = 0; // 256-bit pages

uint8_t bubbleBuffer[bubbleBufferSize];

uint8_t fifo_size = 0;
  
uint8_t mainExecPhase = PHASE_IDLE;

clock_t end_time = 0;

void delayStart(clock_t millis) {
    end_time = clock() + millis;
}

bool isDelayCompleted() {
    return clock() > end_time ? true : false;
}

void delay(int millis) {
    // Storing start time
    clock_t start_time = clock() / (CLOCKS_PER_SEC / 1000);
 
    // looping till required time is not achieved
    while ( (clock() / (CLOCKS_PER_SEC / 1000) ) < (start_time + millis) ) {
        ;
    }
}


uint8_t bubble_read(void* dummy, uint32_t addr) {
    uint8_t value = 0;
    static bool last = false;

    addr = addr - baseAddress;
    addr = addr >> 1;

    //if (isDelayCompleted() == true) {
        //commandEnd(true);
    //}

    //debug_log(DEBUG_DETAIL, "[i7220] Read port: 0x%01X\n", addr);
    
    switch (addr) {
        case 0:
#ifdef DEBUG_BUBBLEMEM
            debug_log(DEBUG_DETAIL, "[i7220] Read register %s\n", regNamesChar[regRAC]);
#endif
            last = false;
            switch (regRAC) {
                case regUtility:
                case regAddressLSB:
                case regAddressMSB:
                    value = regArray[regRAC];
                    regRAC++;
                    regRAC &= 0xF;
                    break;
                case regFIFO:
                    //value = fifo_pop();
                    value = regArray[regFIFO];
                    debug_log(DEBUG_DETAIL, "[i7220] Read FIFO register 0x%02X\n", value);
                    //bitClear(regStatus, BIT_FIFO_READY);
                    commandReadBubbleData();
                    break;
                default:
                    debug_log(DEBUG_DETAIL, "[i7220] Read unknown register 0x%01X\n", regRAC);
                    break;
            }
            break;
        case 1:
            if (last == false) {
                debug_log(DEBUG_DETAIL, "[i7220] Read status register 0x%02X\n", regStatus);
            }
            value = regStatus;
            last = true;
            break;
    }
    return value;
}

void bubble_write(void* dummy, uint32_t addr, uint8_t value) {
    addr = addr - baseAddress;
    addr = addr >> 1;

    //debug_log(DEBUG_DETAIL, "[i7220] Write port: 0x%01X\t value: 0x%02X\n", addr, value);
    switch (addr) {
        case 0:
            if (regRAC) {
                    regArray[regRAC] = value;
#ifdef DEBUG_BUBBLEMEM
                    debug_log(DEBUG_DETAIL, "[i7220] Set reg: %s\t0x%02X\n", regNamesChar[regRAC], value);
#endif
                    regUpdate();
                    regRAC++;
                    regRAC &= 0xF;
                    break;
            } else {
                    //fifo_push(value);
                    debug_log(DEBUG_DETAIL, "[i7220] Computer push into FIFO: 0x%02X\n", value);
                    ;
            }
            break;
        case 1:
            if (bitRead(value, 4)) {
                bubbleCommand = value & 0b00001111;

                //mainExecPhase = PHASE_CMD;
#ifdef DEBUG_BUBBLEMEM
                //debug_log(DEBUG_INFO, "[i7220] Exec command %s\n", bubbleCommands[bubbleCommand]);
#endif
                commandStart(bubbleCommand);
            } else {
                regRAC = value & 0b00001111;
                debug_log(DEBUG_DETAIL, "[i7220] Set RAC: 0x%02X\n", regRAC);
            }
            break;
    }
}

uint8_t bubble_init(I8259_t* i8259) {
    if (bubbleFile != NULL) {
        fclose(bubbleFile);
    }
    bubbleFile = fopen("ROMS/bubble.img", "rwb");
    if (bubbleFile == NULL) {
        debug_log(DEBUG_INFO, "[i7220] Error openimg bubble image\r\n");
        return -1;
    }

    irq8259 = i8259;
    
    memory_mapCallbackRegister(baseAddress, addressLen, (void*)bubble_read, (void*)bubble_write, NULL);
    //int ret = 0;
    //ret = pthread_create(&bubble_ThreadID, NULL, bubble_thread, NULL);
    //return ret;
    return 0;
}

void regUpdate() {
    //int regNFC_my;
    regBlockLen = (regArray[regBlockLenMSB] << 8) + regArray[regBlockLenLSB];
    regAddress = (regArray[regAddressMSB] << 8) + regArray[regAddressLSB];
    
    regBlockLenCount = regBlockLen & 0x7ff;
    regBlockLen_nfc = (regBlockLen >> 12) ? ((regBlockLen >> 12) << 1) : 1;
    //regNFC_my = (regArray[regAddressMSB] & 0xF0) >> 4;
    
    //debug_log(DEBUG_DETAIL, "[i7220] (MAME) Set regBlockLen_nfc: 0x%02x\n", regBlockLen_nfc);
    //debug_log(DEBUG_DETAIL, "[i7220] ( MY ) Set regBlockLen_nfc: 0x%02x\n", regNFC_my);
    regAddress_addr = regAddress & 0x7ff;
    regAddress_mbm = (regAddress >> 11) & 15;
}

void commandStart (uint8_t cmd) {

    //if (isDelayCompleted() == true && mainExecPhase != PHASE_EXEC) {
        //delayStart(delayTime);
    //}
    
    mainExecPhase = PHASE_EXEC;

    bitSet(regStatus,BIT_BUSY);
    bitClear(regStatus, BIT_OP_COMPLETE);

#ifdef DEBUG_BUBBLEMEM
    debug_log(DEBUG_DETAIL, "[i7220] Command: %s\n", bubbleCommands[cmd]);
#endif

    switch ( cmd ) {
        case cmdInitialize:
            // determines how many FSAs are present, reads and decodes
            // bootloop from each MBM and stores result in FSA bootloop
            // regs.
            // all parametric registers must be properly set up before
            // issuing Initialize command.
            // NFC bits in BLR MSB must be set to 0001 before issuing
            // this command.
            // MBM GROUP SELECT bits in the AR must select the last MBM
            // in the system.
            regUpdate();
            if (regBlockLen_nfc != 2) { // maybe constant must be 2
                commandEnd(false);
            } else {
                //if (isDelayCompleted() == true) {
                    commandEnd(true);
                //}
            }
            break;
        case cmdAbort:
            // controlled termination of currently executing command.
            // command accepted in BUSY state.
            // if not BUSY, clears FIFO.
            //fifo_clear();
                //if (isDelayCompleted() == true) {
                    commandEnd(true);
                //}
            break;
        case cmdResetFifo:
            //fifo_clear();
                //if (isDelayCompleted() == true) {
                    commandEnd(true);
                //}
            break;
        case cmdSoftReset:
            // clears BMC FIFO and all registers except those containing init parameters.  sends Reset to every FSA.
            regArray[regUtility] = 0;
            //fifo_clear();
                //if (isDelayCompleted() == true) {
                    commandEnd(true);
                //}
            break;
        case cmdReadBubbleData:
            if (regAddress_mbm >= BUBBLEMODULECOUNT || regBlockLen_nfc != 2) {
                commandEnd(false);
            }
            else {
                lastCommand = cmd;
                commandReadBubbleData();
                //if (isDelayCompleted() == true) {
                    //commandEnd(true);
                    //commandEnd(false);      // for debug: let's pretend fail to read
                //}
            }
            break;
        default :
            debug_log(DEBUG_INFO, "[i7220] command not implemented: 0x%02X\n", cmd);
                //if (isDelayCompleted() == true) {
                    commandEnd(true);
                //}
            break;
    }
}

void commandEnd(uint8_t sucess) {
    //if ( sucess == false) {
        debug_log(DEBUG_INFO, "[i7220] command complete. Status: %s\n", sucess ? "OK" : "FAIL");
    //}
    bitSet(regStatus, sucess ? BIT_OP_COMPLETE : BIT_OP_FAIL);
    bitClear(regStatus, BIT_BUSY);
    //bitClear(regStatus, BIT_OP_COMPLETE);
    mainExecPhase = PHASE_IDLE;
}

void commandReadBubbleData() {

    //if (isDelayCompleted() == true) {
        delayStart(delayTime);
    //}

    if (mainExecPhase != PHASE_EXEC) {
        return;
    }
    
    long filePosition;
    if ( lastCommand != cmdReadBubbleData ) {
        pageCounter = 0; // 256-bit pages
        fifoCounter = 0;
        regBubbleLimit = regBlockLenCount * regBlockLen_nfc;
    }
    
    filePosition = (regAddress_addr * 32 * regBlockLen_nfc) + \
        (regAddress_mbm * I7110_MBM_SIZE) + (pageCounter * 32) + \
        fifoCounter;

#ifdef DEBUG_BUBBLEMEM
        debug_log(DEBUG_DETAIL, "[i7220] Seek to %08X, len: 0x%03X\n", filePosition, regBubbleLimit * 32);
        debug_log(DEBUG_DETAIL, "[i7220] pagecounter %04X, fifocounter: 0x%02X\n", pageCounter, fifoCounter);;
#endif
    fseek(bubbleFile, filePosition, SEEK_SET);
    //uint8_t bubbleByte;
    (void)fread(bubbleBuffer, 1, 1, bubbleFile);
    //(void)fread(bubbleByte, 1, 1, bubbleFile);

    //regArray[regFIFO] = bubbleByte;
    regArray[regFIFO] = bubbleBuffer[0];
    //update_drq(irq8259);
    i8259_setirq(irq8259, 1,1);
    bitSet(regStatus, BIT_FIFO_READY);
    fifoCounter++;
    if (fifoCounter > 31) {
        pageCounter++;
        fifoCounter = 0;
    }
    
    if (pageCounter >= regBubbleLimit) {
        //ramDump(0x20000, 2048);
        commandEnd(true);
    }
    //sectorRead();
    
    //while (pageCounter < regBubbleLimit ) {
        //fseek(bubbleFile, filePosition, SEEK_SET);
        //sectorRead();
        //for (int a = 0; a < 32; a++) {
            //fifo_push(bubbleBuffer[a]);
        //}
        //pageCounter++;
        //delay(delayTime); // p. 4-14 of BPK72UM
        //sectorRead();
    //}
}

//void sectorRead() {
        //(void)fread(bubbleBuffer, 32, 1, bubbleFile);
        //(void)fread(bubbleBuffer, 1, 1, bubbleFile);
//}

void update_drq(I8259_t* i8259) {
        switch (bubbleCommand) {
            case cmdReadBubbleData:
                i8259_setirq(irq8259, 1, fifo_size < 22 ? false : true);
                //i8259_setirq(i8259, 1, fifo_size < 22 ? false : true);
                break;
            case cmdWriteBubbleData:
                i8259_setirq(irq8259, 1, fifo_size < (40 - 22) ? true : false);
                //i8259_setirq(i8259, 1, fifo_size < (40 - 22) ? true : false);
                break;
            default:
                break;
        }
}
