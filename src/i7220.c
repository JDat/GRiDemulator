/*
  GRiD emulator.
  Copyright (C)2022 JDat

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
#include <string.h>
#include <pthread.h>
#include <time.h>
#include "config.h"
#include "i7220.h"
#include "memory.h"
#include "utility.h"
#include "debuglog.h"


//#include <time.h>

#define baseAddress 0xDFE80
#define addressLen  0x3

#define bubbleModuleCount 3

#define I7110_MBM_SIZE (128 * 1024) // 1 megabit
#define I7115_MBM_SIZE (512 * 1024) // 4 megabit

void *bubble_thread(void* cpu);

void fifo_clear();
void fifo_push(uint8_t val);
uint8_t fifo_pop();
void commandStart (uint8_t cmd);
void commandEnd(uint8_t sucess);
void regUpdate();

void commandReadBubbleData();
void sectorRead();
void update_drq();

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
    static const char *regNames[] = {
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


I8259_t* irq8259;

volatile uint8_t regRAC;
volatile uint8_t regArray[6];
volatile uint8_t regStatus;

#define BUBBLEFIFOSIZE 40
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

#define bubbleBufferSize 32
uint8_t bubbleBuffer[32];
  
enum cmdStates{
    PHASE_IDLE, PHASE_CMD, PHASE_EXEC, PHASE_RESULT
};
volatile uint8_t mainExecPhase;

uint8_t bubble_read(void* dummy, uint32_t addr) {
    addr = addr - baseAddress;
    addr = addr >> 1;
#ifdef DEBUG_BUBBLEMEM
    debug_log(DEBUG_DETAIL, "[i7220] Read port 0x%02X\n", addr);
#endif
    uint8_t value = 0;
    
    switch (addr) {
        case 0:
            switch (regRAC) {
                case regUtility:
                case regAddressLSB:
                case regAddressMSB:
                    value = regArray[regRAC - 10];
#ifdef DEBUG_BUBBLEMEM
                    debug_log(DEBUG_DETAIL, "[i7220] Read register: 0x%02x == 0x%02x\tName: %s\n", regRAC, value, regNames[regRAC - 10]);
#endif
                    regRAC++;
                    regRAC &= 0xF;
                    break;
                case regFIFO:
                    value = fifo_pop();
#ifdef DEBUG_BUBBLEMEM
                    debug_log(DEBUG_DETAIL, "[i7220] Read FIFO register: 0x%02x\n", value);
#endif
                    break;
                default:
#ifdef DEBUG_BUBBLEMEM
                    debug_log(DEBUG_DETAIL, "[i7220] Read register error: 0x%02x == 0x%02x\n", regRAC, value);
#endif
                    break;
            }
            break;
        case 1:
            value = regStatus;
#ifdef DEBUG_BUBBLEMEM
            debug_log(DEBUG_DETAIL, "[i7220] Read status register: 0x%02x\n", value);
#endif
            break;
    }
    return value;
}

void bubble_write(void* dummy, uint32_t addr, uint8_t value) {
    addr = addr - baseAddress;
    addr = addr >> 1;
#ifdef DEBUG_BUBBLEMEM
    debug_log(DEBUG_DETAIL, "[i7220] Write port 0x%02x: 0x%02x\n", addr, value);
#endif
    switch (addr) {
        case 0:
            if (regRAC) {
                    regArray[regRAC - 10] = value;
#ifdef DEBUG_BUBBLEMEM
                    debug_log(DEBUG_DETAIL, "[i7220] Write register: 0x%02x == 0x%02x\tName: %s\n", regRAC, value, regNames[regRAC - 10]);
#endif
                    regUpdate();
                    regRAC++;
                    regRAC &= 0xF;
                    break;
            } else {
                    fifo_push(value);
#ifdef DEBUG_BUBBLEMEM
                    debug_log(DEBUG_DETAIL, "[i7220] write FIFO register: 0x%02x\n", value);
#endif
            }
            break;
        case 1:
            if (bitRead(value, 4)) {
                bubbleCommand = value & 15;
                //if (mainExecPhase == PHASE_IDLE) {
#ifdef DEBUG_BUBBLEMEM
                    debug_log(DEBUG_INFO, "[i7220] Write command: 0x%02x\t Name: %s\n", value, bubbleCommands[bubbleCommand]);
#endif
                    bitSet(regStatus,BIT_BUSY);
                    bitClear(regStatus,BIT_OP_COMPLETE);
                    mainExecPhase = PHASE_CMD;
                //}
            } else {
                regRAC = value & 0b00001111;
            }
            break;
    }
}

//void bubble_init() {
uint8_t bubble_init(I8259_t* i8259) {
    if (bubbleFile != NULL) {
        fclose(bubbleFile);
    }
    bubbleFile = fopen("ROMS/bubble.img", "rwb");
    if (bubbleFile == NULL) {
#ifdef DEBUG_BUBBLEMEM
        debug_log(DEBUG_INFO, "[i7220] Error openimg bubble image\r\n");
#endif
    return -1;
    }

    irq8259 = i8259;
    
#ifdef DEBUG_BUBBLEMEM
    debug_log(DEBUG_INFO, "[i7220] Initializing bubble memory controller\r\n");
#endif
    memory_mapCallbackRegister(baseAddress, addressLen, (void*)bubble_read, (void*)bubble_write, NULL);
    int ret;
    ret = pthread_create(&bubble_ThreadID, NULL, bubble_thread, NULL);

#ifdef DEBUG_BUBBLEMEM
    debug_log(DEBUG_INFO, "[i7220] Thread_create returned %d\r\n", ret);
#endif
    return ret;
}

void *bubble_thread(void* dummy) {
//void bubble_thread() {
    
#ifdef DEBUG_BUBBLEMEM
    debug_log(DEBUG_INFO, "[i7220] Thread function begin\n");
#endif
    while (true) {
            //debug_log(DEBUG_INFO, "[i7220] Thread while loop\n");
            if (mainExecPhase == PHASE_CMD ) {
#ifdef DEBUG_BUBBLEMEM
                debug_log(DEBUG_INFO, "[i7220] Thread command %02x '%s'\n", bubbleCommand, bubbleCommands[bubbleCommand]);
#endif
                commandStart(bubbleCommand);
                lastCommand = bubbleCommand;
            }
    }

    pthread_exit(NULL);
#ifdef DEBUG_BUBBLEMEM
    debug_log(DEBUG_INFO, "[i7220] Thread end\r\n");
#endif
}

void fifo_clear() {
#ifdef DEBUG_BUBBLEMEM
        debug_log(DEBUG_INFO, "[i7220] fifo clear\n");
#endif
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
//#ifdef DEBUG_BUBBLEMEM
    //debug_log(DEBUG_INFO, "[i7220] fifo pop: 0x%02x, FIFO size: %d\n", val, fifo_size);
//#endif
    if (fifo_head == fifo_tail) {
        bitClear(regStatus,BIT_FIFO_READY);
    }
    fifoLock = false;
    return val;
}

void fifo_push(uint8_t val) {    
    //if ( fifo_size >= (BUBBLEFIFOSIZE - 1) ) {
    //    debug_log(DEBUG_INFO, "[i7220] fifo push: buffer full\n");
    //}
    while ( fifo_size >= (BUBBLEFIFOSIZE - 1)  ) {
        ;
    }

    while ( fifoLock == true) {
        ;
    }
    
    fifoLock = true;

//#ifdef DEBUG_BUBBLEMEM
    //debug_log(DEBUG_INFO, "[i7220] fifo push: 0x%02x, FIFO size: %d\n", val, fifo_size);
//#endif
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
}

void delay(int millis) {
    // Storing start time
    clock_t start_time = clock();
 
    // looping till required time is not achieved
    while (clock() < start_time + millis) {
        ;
    }
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
#ifdef DEBUG_BUBBLEMEM
            debug_log(DEBUG_INFO, "[i7220] commandInit: BLR 0x%04x (NFC %d pages %d) AR 0x%04x (MBM %d addr 0x%03x) ER 0x%02x\n",
                regBlockLen, regBlockLen_nfc, regBlockLenCount, regAddress, regAddress_mbm, regAddress_addr, regArray[regEnable-10]);
#endif
            if (regBlockLen_nfc != 2) { // maybe constant must be 2
                commandEnd(false);
            } else {
                //delay(bubbleModuleCount * 60);
                delay(4);
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
            break;
        case cmdSoftReset:
            // clears BMC FIFO and all registers except those containing init parameters.  sends Reset to every FSA.
            regArray[regUtility] = 0;
            fifo_clear();
            commandEnd(true);
            break;
        case cmdReadBubbleData:
#ifdef DEBUG_BUBBLEMEM
            debug_log(DEBUG_INFO, "[i7220] commandReadBubbleData: BLR 0x%04x (NFC %d pages %d) AR 0x%04x (MBM %d addr 0x%03x) ER 0x%02x\n",
                regBlockLen, regBlockLen_nfc, regBlockLenCount, regAddress, regAddress_mbm, regAddress_addr, regArray[regEnable-10]);
#endif

            if (regAddress_mbm >= bubbleModuleCount || regBlockLen_nfc != 2) {
                commandEnd(false);
            }
            else {
                commandReadBubbleData();
                commandEnd(true);
            }
            break;
        default :
//#ifdef DEBUG_BUBBLEMEM
            debug_log(DEBUG_INFO, "[i7220] command not implemented: 0x%02X\n", cmd);
//#endif
            commandEnd(false);
            break;
    }
}

void commandEnd(uint8_t sucess) {
#ifdef DEBUG_BUBBLEMEM
    debug_log(DEBUG_INFO, "[i7220] command complete. Status: %s\n", sucess ? "OK" : "FAIL");
#endif
    if ( sucess == false) {
        debug_log(DEBUG_INFO, "[i7220] command complete. Status: FAIL\n");
    }
    bitSet(regStatus, sucess ? BIT_OP_COMPLETE : BIT_OP_FAIL);
    bitClear(regStatus, BIT_BUSY);
    mainExecPhase = PHASE_IDLE;
}

void commandReadBubbleData() {
    if ( lastCommand != cmdReadBubbleData ) {
        regBubbleCounter = 0; // 256-bit pages
        regBubbleLimit = regBlockLenCount * regBlockLen_nfc;
    }
//#ifdef DEBUG_BUBBLEMEM
    debug_log(DEBUG_INFO, "[i7220] readbubble: seek file pos: %d\n", (regAddress_addr * 32 * regBlockLen_nfc) + (regAddress_mbm * I7110_MBM_SIZE) + (regBubbleCounter * 32)) ;
//#endif
    fseek(bubbleFile, (regAddress_addr * 32 * regBlockLen_nfc) + (regAddress_mbm * I7110_MBM_SIZE) + (regBubbleCounter * 32), SEEK_SET);
    sectorRead();
    
    while (regBubbleCounter < regBubbleLimit ) {
        for (int a = 0; a < 32; a++) {
            fifo_push(bubbleBuffer[a]);
        }
        //m_bi.sub_state = SECTOR_READ;
        regBubbleCounter++;
        //delay_cycles(m_bi.tm, 270 * 20); // p. 4-14 of BPK72UM
        delay(4); // p. 4-14 of BPK72UM
        sectorRead();
    }
}

void sectorRead() {
        size_t ret;
        //debug_log(DEBUG_INFO, "[i7220] [file access] read data. ftell: %d\n", ftell(bubbleFile) );
        ret = fread(bubbleBuffer, 32, 1, bubbleFile);
        //delay(4); // p. 4-14 of BPK72UM
}

void update_drq() {

    //if ( bitRead(regArray[regEnable-10], BIT_DMA_ENABLED) ) {
        switch (bubbleCommand) {
            case cmdReadBubbleData:
                //set_drq(fifo_size < 22 ? false : true);
                i8259_setirq(irq8259, 1, fifo_size < 22 ? false : true);
                //if (fifo_size >= 22) {
                //    i8259_doirq(irq8259, 1);
                //}
                break;
            case cmdWriteBubbleData:
                //set_drq(fifo_size < (40 - 22) ? true : false);
                i8259_setirq(irq8259, 1, fifo_size < (40 - 22) ? true : false);
                //if (fifo_size < (40 - 22)) {
                //    i8259_doirq(irq8259, 1);
                //}
            break;
        }
    //} else {
    //    i8259_setirq(irq8259, 1, false);
    //}
}
