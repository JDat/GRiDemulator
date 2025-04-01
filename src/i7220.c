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
#include "i8259.h"
#include "memory.h"
#include "utility.h"
#include "debuglog.h"

#define delayTime 4

#define baseAddress 0xDFE80
#define addressLen  0x4

#define bubbleModuleCount 3

#define bubbleBufferSize 32
#define BUBBLEFIFOSIZE 40


#define I7110_MBM_SIZE (128 * 1024) // 1 megabit
#define I7115_MBM_SIZE (512 * 1024) // 4 megabit


void fifo_clear();
void fifo_push(uint8_t val);
uint8_t fifo_pop();
void commandStart (uint8_t cmd);
void commandEnd(uint8_t sucess);
void regUpdate();

void commandReadBubbleData();
void sectorRead();
void update_drq();

void *bubble_thread(void* cpu);
pthread_t bubble_ThreadID;

FILE *bubbleFile;

enum regNames{
    regFIFO         = 0x00,
    regUtility      = 0x0a,
    regBlockLenLSB  = 0x0b,
    regBlockLenMSB  = 0x0c,
    regEnable       = 0x0d,
    regAddressLSB   = 0x0e,
    regAddressMSB   = 0x0f,
} ;

#ifdef DEBUG_BUBBLEMEM
    static const char *regNamesChar[] = {
        "Utility register",
        "Block length register LSB",
        "Block length register MSB",
        "Enable register",
        "Address register LSB",
        "Address register MSB",
    };
#endif

enum cmdNames{
    cmdWrBootLoopRegMasked = 0,
    cmdInitialize = 1,
    cmdReadBubbleData = 2,
    cmdWriteBubbleData = 3,
    cmdReadSeek = 4,
    cmdRdBootLoopReg = 5,
    cmdWrBootLoopReg = 6,
    cmdWrBootLoop = 7,
    cmdRdFsaStatus= 8,
    cmdAbort = 9,
    cmdWriteSeek = 10,
    cmdRdBootLoop = 11,
    cmdRdCorrectedData = 12,
    cmdResetFifo = 13,
    cmdMbmPurge = 14,
    cmdSoftReset = 15,
};

#ifdef DEBUG_BUBBLEMEM
    static const char *bubbleCommands[] = {
        "Write Bootloop Register Masked",
        "Initialize",
        "Read Bubble Data",
        "Write Bubble Data",
        "Read Seek",
        "Read BootLoop Register",
        "Write BootLoop Register",
        "Write BootLoop",
        "Read FSA Status",
        "Abort",
        "Write Seek",
        "Read BootLoop",
        "Read Corrected Data",
        "Reset FIFO",
        "MBM Purge",
        "Software Reset",
    };
#endif

enum statusBits {
    BIT_FIFO_READY          = 0,
    BIT_PARITY_ERROR        = 1,
    BIT_UNCORRECTABLE_ERROR = 2,
    BIT_CORRECTABLE_ERROR   = 3,
    BIT_TIMING_ERROR        = 4,
    BIT_OP_FAIL             = 5,
    BIT_OP_COMPLETE         = 6,
    BIT_BUSY                = 7,
};

enum enableBits {
    BIT_IRQ_NORMAL          = 0,
    BIT_IRQ_ERROR           = 1,
    BIT_DMA_ENABLED         = 2,
    BIT_MAX_FSA_XFER_RATE   = 3,
    BIT_WRITE_BOOTLOOP_ENA  = 4,
    BIT_RCD_ENA             = 5,
    BIT_ICD_ENA             = 6,
    BIT_PARITY_INTERRUPT    = 7,
};

//I8259_t* irq8259;

volatile uint8_t regRAC;
volatile uint8_t regArray[6];
volatile uint8_t regStatus;

volatile int8_t fifo_size, fifo_head, fifo_tail;
volatile uint8_t fifoArray[BUBBLEFIFOSIZE];
volatile uint8_t fifoLock = false;

volatile uint8_t bubbleCommand;
volatile uint8_t lastCommand;

int regBlockLen;
int regAddress;
int regBlockLenCount;
int regBlockLen_nfc;
int regAddress_addr;
int regAddress_mbm;
int regBubbleCounter; // 256-bit pages
int regBubbleLimit;

int fifoCount = 0;

uint8_t bubbleBuffer[bubbleBufferSize];
  
enum cmdStates{
    PHASE_IDLE, PHASE_CMD, PHASE_EXEC, PHASE_RESULT
};

volatile uint8_t mainExecPhase;

void delay(int millis) {
    // Storing start time
    clock_t start_time = clock() / (CLOCKS_PER_SEC / 1000);
 
    // looping till required time is not achieved
    while ( (clock() / (CLOCKS_PER_SEC / 1000) ) < (start_time + millis) ) {
        ;
    }
}

uint8_t bubble_read(void* dummy, uint32_t addr) {
    addr = addr - baseAddress;
    addr = addr >> 1;
//#ifdef DEBUG_BUBBLEMEM
    //debug_log(DEBUG_DETAIL, "[i7220] Read port 0x%02X\n", addr);
//#endif
    uint8_t value = 0;
    
    switch (addr) {
        case 0:
#ifdef DEBUG_BUBBLEMEM
    if (regRAC == 0 ) {
        //debug_log(DEBUG_DETAIL, "[i7220] Read FIFO reg: %d\n", abs(fifo_head - fifo_tail));
        ;
    } else {
        debug_log(DEBUG_DETAIL, "[i7220] Read reg %s\n", regNamesChar[regRAC - 10]);
    }
#endif
            switch (regRAC) {
                case regUtility:
                case regAddressLSB:
                case regAddressMSB:
                    value = regArray[regRAC - 10];
                    regRAC++;
                    regRAC &= 0xF;
                    break;
                case regFIFO:
                    value = fifo_pop();
                    //debug_log(DEBUG_DETAIL, "[i7220] FIFO value 0x%02X\n", value);
                    //debug_log(DEBUG_DETAIL, "[i7220] FIFO Size %d\n", fifo_size);
                    //debug_log(DEBUG_DETAIL, "[i7220] status Reg 0x%02X\n", regStatus);
                    //debug_log(DEBUG_DETAIL, "%02X ", value);
                    //if (fifoCount >= 15) {
                        //fifoCount = 0;
                        //debug_log(DEBUG_DETAIL, "\n");
                        
                    //} else {
                        //debug_log(DEBUG_DETAIL, "%02X ", value);
                        //fifoCount++;
                    //}
                    break;
                default:
                    debug_log(DEBUG_DETAIL, "[i7220] Read unknown register 0x%01X\n", regRAC);
                    break;
            }
            break;
        case 1:
//#ifdef DEBUG_BUBBLEMEM
    //debug_log(DEBUG_DETAIL, "[i7220] Read status 0x%02X\n", regStatus);
//#endif
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
                    #ifdef DEBUG_BUBBLEMEM
                        debug_log(DEBUG_DETAIL, "[i7220] Write reg %s: 0x%02X\n", regNamesChar[regRAC - 10], value);
                        //debug_log(DEBUG_DETAIL, "[i7220] Write reg 0x%01X: 0x%02X\n", regRAC, value);
                    #endif
                    regArray[regRAC - 10] = value;
                    regUpdate();
                    regRAC++;
                    regRAC &= 0xF;
                    break;
            } else {
                    fifo_push(value);
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

//uint8_t bubble_init(I8259_t* i8259) {
uint8_t bubble_init() {
    if (bubbleFile != NULL) {
        fclose(bubbleFile);
    }
    bubbleFile = fopen("ROMS/bubble.img", "rwb");
    if (bubbleFile == NULL) {
        debug_log(DEBUG_INFO, "[i7220] Error openimg bubble image\r\n");
    return -1;
    }

    //irq8259 = i8259;
    
#ifdef DEBUG_BUBBLEMEM
    debug_log(DEBUG_INFO, "[i7220] Initializing bubble memory controller\r\n");
#endif
    memory_mapCallbackRegister(baseAddress, addressLen, (void*)bubble_read, (void*)bubble_write, NULL);
    int ret;
    ret = pthread_create(&bubble_ThreadID, NULL, bubble_thread, NULL);
    return ret;
}

void *bubble_thread(void* dummy) {
    while (true) {
            if (mainExecPhase == PHASE_CMD ) {
                commandStart(bubbleCommand);
                lastCommand = bubbleCommand;
            }
    }

    pthread_exit(NULL);
}

void fifo_clear() {
    fifo_size = 0;
    fifo_head = fifo_tail = 0;
    bitClear(regStatus,BIT_FIFO_READY);
    update_drq();
    
}

uint8_t fifo_pop() {
    while (fifo_head == fifo_tail) {
        ;
    }

    while ( fifoLock == true) {
        ;
    }
    
    fifoLock = true;

    if (fifo_head == fifo_tail) {
        bitClear(regStatus,BIT_FIFO_READY);
        debug_log(DEBUG_INFO, "[i7220] fifo pop: this is bad\n");
        fifoLock = false;
        return 0;
    }
    
    uint8_t val;
    val = fifoArray[fifo_tail];
    fifo_tail = (fifo_tail + 1) % BUBBLEFIFOSIZE;
    if (mainExecPhase == PHASE_EXEC) {
            update_drq();
    }
    fifo_size --;
    if (fifo_head == fifo_tail) {
        bitClear(regStatus,BIT_FIFO_READY);
    }
    fifoLock = false;
    return val;
    //delay(delayTime);
}

void fifo_push(uint8_t val) {    
    while ( fifo_size >= (BUBBLEFIFOSIZE - 1)  ) {
        ;
    }

    while ( fifoLock == true) {
        ;
    }
    
    fifoLock = true;

    uint8_t i;
    i = (fifo_head + 1) % BUBBLEFIFOSIZE;
    if (i != fifo_tail) {
            fifoArray[fifo_head] = val;
            fifo_head = i;
            fifo_size ++;
            bitSet(regStatus,BIT_FIFO_READY);
            if (mainExecPhase == PHASE_EXEC) {
                    update_drq();
            }
    }
    
    fifoLock = false;
    //delay(delayTime);
}

void regUpdate() {
    regBlockLen = (regArray[regBlockLenMSB - 10] << 8) + regArray[regBlockLenLSB - 10];
    regAddress = (regArray[regAddressMSB - 10] << 8) + regArray[regAddressLSB - 10];
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
                delay(delayTime);
                commandEnd(true);
            }
            break;
        case cmdAbort:
            // controlled termination of currently executing command.
            // command accepted in BUSY state.
            // if not BUSY, clears FIFO.
            fifo_clear();
            commandEnd(true);
            break;
        case cmdResetFifo:
            fifo_clear();
            commandEnd(true);
            fifoCount = 0;
            break;
        case cmdSoftReset:
            // clears BMC FIFO and all registers except those containing init parameters.  sends Reset to every FSA.
            regArray[regUtility] = 0;
            fifo_clear();
            commandEnd(true);
            break;
        case cmdReadBubbleData:
            if (regAddress_mbm >= bubbleModuleCount || regBlockLen_nfc != 2) {
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
        regBubbleCounter = 0; // 256-bit pages
        regBubbleLimit = regBlockLenCount * regBlockLen_nfc;
    }
    
    filePosition = (regAddress_addr * 32 * regBlockLen_nfc) + (regAddress_mbm * I7110_MBM_SIZE) + (regBubbleCounter * 32);
#ifdef DEBUG_BUBBLEMEM
        debug_log(DEBUG_DETAIL, "[i7220] Seek to %08X, len: 0x%03X\n", filePosition, regBubbleLimit * 32);
#endif
    fseek(bubbleFile, filePosition, SEEK_SET);
    //sectorRead();
    
    while (regBubbleCounter < regBubbleLimit ) {
        //fseek(bubbleFile, filePosition, SEEK_SET);
        sectorRead();
        for (int a = 0; a < 32; a++) {
            fifo_push(bubbleBuffer[a]);
        }
        regBubbleCounter++;
        delay(delayTime); // p. 4-14 of BPK72UM
        //sectorRead();
    }
}

void sectorRead() {
        (void)fread(bubbleBuffer, 32, 1, bubbleFile);
}

void update_drq() {
        //while (insideInterrupt == true) {
        //    ;
        //}
        
        switch (bubbleCommand) {
            case cmdReadBubbleData:
                i8259_setirq(irqBubble, fifo_size > 22 ? true : false);
                //i8259_setirq(irqBubble, fifo_size < 22 ? false : true);
                break;
            case cmdWriteBubbleData:
                i8259_setirq(irqBubble, fifo_size < (40 - 22) ? true : false);
                //i8259_setirq(irqBubble, fifo_size < (40 - 22) ? false : true);
            break;
        }
}
