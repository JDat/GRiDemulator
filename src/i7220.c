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
#include <string.h>
#include <pthread.h>
#include <time.h>
#include "config.h"
#include "i7220.h"
#include "memory.h"
#ifdef DEBUG_BUBBLEMEM
    #include "debuglog.h"
#endif
#include "utility.h"
//#include <time.h>

#define baseAddress 0xDFE80
#define addressLen  0x3

void *bubble_thread(void* cpu);

void fifo_clear();
void fifo_push(uint8_t val);
uint8_t fifo_pop();

pthread_t bubble_ThreadID;

enum {
    regFIFO         = 0x00,
    regUtility      = 0x0a,
    regBlockLenLSB  = 0x0b,
    regBlockLenMSB  = 0x0c,
    regEnable       = 0x0d,
    regAddressLSB   = 0x0e,
    regAddressMSB   = 0x0f,
};

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

enum {
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

uint8_t regRAC;
uint8_t regArray[6];
uint8_t regStatus;

#define BUBBLEFIFOSIZE 40
int m_fifo_size;
uint8_t m_fifo[BUBBLEFIFOSIZE];
int m_fifo_head, m_fifo_tail;

uint8_t bubbleCommand;

enum {
    PHASE_IDLE, PHASE_CMD, PHASE_EXEC, PHASE_RESULT
};
uint8_t mainExecPhase;

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
                    debug_log(DEBUG_DETAIL, "[i7220] Read register: %02x == %02x\tName: %s\n", regRAC, value, regNames[regRAC - 10]);
                    regRAC++;
                    regRAC &= 0xF;
                    break;
                case regFIFO:
                    value = fifo_pop();
                    debug_log(DEBUG_DETAIL, "[i7220] Read FIFO register: %02x\n", value);
                    break;
                default:
                    debug_log(DEBUG_DETAIL, "[i7220] Read register error: %02x == %02x\n", regRAC, value);
                    break;
            }
            break;
        case 1:
            value = regStatus;
            debug_log(DEBUG_DETAIL, "[i7220] Read status register: %02x\n", value);
            break;
    }
    return value;
}

void bubble_write(void* dummy, uint32_t addr, uint8_t value) {
    addr = addr - baseAddress;
    addr = addr >> 1;
#ifdef DEBUG_BUBBLEMEM
    debug_log(DEBUG_DETAIL, "[i7220] Write port 0x%02X: %02X\n", addr, value);
#endif
    switch (addr) {
        case 0:
            if (regRAC) {
                    regArray[regRAC - 10] = value;
                    debug_log(DEBUG_DETAIL, "[i7220] Write register: %02x == %02x\tName: %s\n", regRAC, value, regNames[regRAC - 10]);
                    regRAC++;
                    regRAC &= 0xF;
                    break;
            } else {
                    fifo_push(value);
                    debug_log(DEBUG_DETAIL, "[i7220] write FIFO register: %02x\n", value);
            }
            break;
        case 1:
            if (bitRead(value, 4)) {
                bubbleCommand = value & 15;
                //if (mainExecPhase == PHASE_IDLE) {
                    debug_log(DEBUG_INFO, "[i7220] Write command: %02x\t Name: %s\n", value, bubbleCommands[bubbleCommand]);

                    //	m_main_phase = PHASE_CMD;
                    //bitSet(m_str,7);

                    //m_main_phase = PHASE_EXEC;
                    //m_str &= ~SR_CLEAR;
                    //m_haveCmd = 1;
                    //	
                    //start_command(m_cmdr);
                //}
            } else {
                regRAC = value & 0b00001111;
            }
            break;
    }
}

//void bubble_init() {
uint8_t bubble_init(I8259_t* i8259) {
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
    uint8_t a;
    //debug_log(DEBUG_INFO, "[i7220] Thread function call\n");
    while (running) {
            //debug_log(DEBUG_INFO, "[i7220] Thread while loop\n");
            a=0;
            //if (m_main_phase == PHASE_IDLE && m_haveCmd == 1)
            //{
                    //debug_log(DEBUG_INFO, "[i7220] Thread command %02x '%s'\n", m_cmdr, commands[m_cmdr]);
                    //m_haveCmd = 0;
                    //m_main_phase = PHASE_CMD;
                    //start_command(m_cmdr);
            //}
    }

    pthread_exit(NULL);
    debug_log(DEBUG_INFO, "[i7220] Thread end\r\n");
}

void fifo_clear() {
	//m_fifo.clear();
#ifdef DEBUG_BUBBLEMEM
        debug_log(DEBUG_INFO, "[i7220] fifo clear\n");
#endif
	m_fifo_size = 0;
	//set_drq(false);
        m_fifo_head = m_fifo_tail = 0;
}

uint8_t fifo_pop() {
    if (m_fifo_head == m_fifo_tail) return 0;
    
    uint8_t val;
    val = m_fifo[m_fifo_tail];
    m_fifo_tail = (m_fifo_tail + 1) % BUBBLEFIFOSIZE;
    //if (m_main_phase == PHASE_EXEC) {
    //        update_drq();
    //}
    m_fifo_size --;
#ifdef DEBUG_BUBBLEMEM
    debug_log(DEBUG_INFO, "[i7220] fifo pop: %02X\n", val);
#endif
    return val;
}

void fifo_push(uint8_t val) {
#ifdef DEBUG_BUBBLEMEM
    debug_log(DEBUG_INFO, "[i7220] fifo push: %02X\n", val);
#endif
    uint8_t i;
    i = (m_fifo_head + 1) % BUBBLEFIFOSIZE;
    if (i != m_fifo_tail) {
            m_fifo[m_fifo_head] = val;
            m_fifo_head = i;
            m_fifo_size ++;
            //if (m_main_phase == PHASE_EXEC)
            //{
            //        update_drq();
            //}
    }
}

void delay(int millis)
{
    // Converting time into milli_seconds
    int milli_seconds = millis;
 
    // Storing start time
    clock_t start_time = clock();
 
    // looping till required time is not achieved
    while (clock() < start_time + milli_seconds)
        ;
}
