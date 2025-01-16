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
#include <pthread.h>
#include <time.h>
#include "config.h"
#include "i7220.h"
#include "i7220_debug.h"
#include "i7220_private.h"
#include "i8259.h"
#include "memory.h"
#include "utility.h"
#include "debuglog.h"

#define delayTime 4

#define baseAddress 0xDFE80
#define addressLen  0x4

#define BUBBLEMODULECOUNT 3

#define bubbleBufferSize 32
#define BUBBLEFIFOSIZE 32


#define I7110_MBM_SIZE (128 * 1024) // 1 megabit
#define I7115_MBM_SIZE (512 * 1024) // 4 megabit

void commandStart (uint8_t cmd);
void commandEnd(uint8_t sucess);
void regUpdate();

void commandReadBubbleData();
void sectorRead();
void update_drq();

pthread_t bubble_ThreadID;
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

uint8_t bubbleBuffer[bubbleBufferSize];
  
volatile uint8_t mainExecPhase;

void delay(int millis) {
    // Storing start time
    clock_t start_time = clock();
 
    // looping till required time is not achieved
    while (clock() < start_time + millis) {
        ;
    }
}

uint8_t bubble_read(void* dummy, uint32_t addr) {
    uint8_t value = 0;

    addr = addr - baseAddress;
    addr = addr >> 1;
    
    switch (addr) {
        case 0:
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
                    break;
                default:
                    debug_log(DEBUG_DETAIL, "[i7220] Read unknown register 0x%01X\n", regRAC);
                    break;
            }
            break;
        case 1:
            value = regStatus;
            break;
    }
    return value;
}

void bubble_write(void* dummy, uint32_t addr, uint8_t value) {
    addr = addr - baseAddress;
    addr = addr >> 1;
    switch (addr) {
        case 0:
            if (regRAC) {
                    regArray[regRAC] = value;
                    regUpdate();
                    regRAC++;
                    regRAC &= 0xF;
                    break;
            } else {
                    //fifo_push(value);
                    ;
            }
            break;
        case 1:
            if (bitRead(value, 4)) {
                bubbleCommand = value & 0b00001111;
                bitSet(regStatus,BIT_BUSY);
                bitClear(regStatus, BIT_OP_COMPLETE);
                mainExecPhase = PHASE_CMD;
            } else {
                regRAC = value & 0b00001111;
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
    regBlockLen = (regArray[regBlockLenMSB] << 8) + regArray[regBlockLenLSB];
    regAddress = (regArray[regAddressMSB] << 8) + regArray[regAddressLSB];
    regBlockLenCount = regBlockLen & 0x7ff;
    regBlockLen_nfc = (regBlockLen >> 12) ? ((regBlockLen >> 12) << 1) : 1;
    regAddress_addr = regAddress & 0x7ff;
    regAddress_mbm = (regAddress >> 11) & 15;
}

void commandStart (uint8_t cmd) {
    mainExecPhase = PHASE_EXEC;

#ifdef DEBUG_BUBBLEMEM
    debug_log(DEBUG_INFO, "[i7220] Command: %s\n", bubbleCommands[cmd]);
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
            if (regBlockLen_nfc != 2) { // maybe constant must be 2
                commandEnd(false);
            } else {
                //delay(delayTime);
                commandEnd(true);
            }
            break;
        case cmdAbort:
            // controlled termination of currently executing command.
            // command accepted in BUSY state.
            // if not BUSY, clears FIFO.
            //fifo_clear();
            commandEnd(true);
            break;
        case cmdResetFifo:
            //fifo_clear();
            commandEnd(true);
            fifoCount = 0;
            break;
        case cmdSoftReset:
            // clears BMC FIFO and all registers except those containing init parameters.  sends Reset to every FSA.
            regArray[regUtility] = 0;
            //fifo_clear();
            commandEnd(true);
            break;
        case cmdReadBubbleData:
            if (regAddress_mbm >= BUBBLEMODULECOUNT || regBlockLen_nfc != 2) {
                commandEnd(false);
            }
            else {
                commandReadBubbleData();
                commandEnd(true);
            }
            break;
        default :
            debug_log(DEBUG_INFO, "[i7220] command not implemented: 0x%02X\n", cmd);
            commandEnd(false);
            break;
    }
}

void commandEnd(uint8_t sucess) {
    if ( sucess == false) {
        debug_log(DEBUG_INFO, "[i7220] command complete. Status: FAIL\n");
    }
    bitSet(regStatus, sucess ? BIT_OP_COMPLETE : BIT_OP_FAIL);
    bitClear(regStatus, BIT_BUSY);
    mainExecPhase = PHASE_IDLE;
}

void commandReadBubbleData() {
    long filePosition;
    if ( lastCommand != cmdReadBubbleData ) {
        pageCounter = 0; // 256-bit pages
        regBubbleLimit = regBlockLenCount * regBlockLen_nfc;
    }
    
    filePosition = (regAddress_addr * 32 * regBlockLen_nfc) + (regAddress_mbm * I7110_MBM_SIZE) + (pageCounter * 32);
#ifdef DEBUG_BUBBLEMEM
        debug_log(DEBUG_DETAIL, "[i7220] Seek to %08X, len: 0x%03X\n", filePosition, regBubbleLimit * 32);
#endif
    fseek(bubbleFile, filePosition, SEEK_SET);
    //sectorRead();
    
    while (pageCounter < regBubbleLimit ) {
        //fseek(bubbleFile, filePosition, SEEK_SET);
        sectorRead();
        for (int a = 0; a < 32; a++) {
            //fifo_push(bubbleBuffer[a]);
        }
        pageCounter++;
        //delay(delayTime); // p. 4-14 of BPK72UM
        //sectorRead();
    }
}

void sectorRead() {
        (void)fread(bubbleBuffer, 32, 1, bubbleFile);
}

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
        }
}
