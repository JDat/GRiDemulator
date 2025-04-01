/*
  GRiD Compass emulator
  Copyright (C)2022 JDat
  https://github.com/JDat/GRiDemulator

  Based on MAMEdev project
  license:BSD-3-Clause
  copyright-holders:Sergey Svishchev
  https://github.com/mamedev/mame/blob/master/src/devices/machine/tms9914.cpp

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
	TMS914A GPIB controller
*/

#include <stdio.h>
#include <stdint.h>
#include <string.h>
#include <stdbool.h>
#include "config.h"
#include "tms9914a.h"
#include "tms9914a_private.h"
#include "i8259.h"
#include "memory.h"
#include "debuglog.h"
#include "utility.h"
#include "GPiB_bus.h"

#include "cpu.h"
//#include <time.h>

#define baseAddress 0xDFF80
#define addressLen  0xF

// Auxiliary commands

uint8_t registers[16];
uint32_t auxBits;
bool irqLine;
uint8_t controllerState;
bool externalStateChange;
bool reflection;

int8_t busMemberId;

void do_aux_cmd(unsigned cmd , bool set_bit) {

  switch(cmd) {
    case AUXCMD_SWRST:
      //fixme
      do_swrst();
      break;
    case AUXCMD_SIC:
      if ( bitRead(auxBits, flagSIC) != set_bit ) {
          bitWrite(auxBits, flagSIC, set_bit);
          //update ifc
          //set_signal(IEEE_488_IFC , m_sic);
          GPiBbusIFCwrite(bitRead(auxBits, flagSIC), busMemberId);
          
          if ( !controller_reset() && bitRead(auxBits, flagSIC) && (controllerState  == FSM_C_CIDS) ) {
            controllerState = FSM_C_CADS;
            externalStateChange = true;
            // need to implement. Dohh! 900 lines of code.
            //update_fsm();
          }
          
      }
      break;
    default:
      break;
  }
#ifdef DEBUG_GPIB
            debug_log(DEBUG_DETAIL, "[GPIB] Flag: %i\t Cmd: 0x%02X\t%s\n", set_bit, cmd, auxCmd[cmd].name);
#endif
}


uint8_t tms9914a_read(void* dummy, uint32_t addr) {
        uint8_t ret;
        addr = addr - baseAddress;
        addr = addr >> 1;
        

#ifdef DEBUG_GPIB
        debug_log(DEBUG_DETAIL, "[GPIB] Read port 0x%02X\n", addr);
#endif

      ret = 0xFF;
      return ret;
      
        switch(addr) {
          case regIntStatus0:
            ret = registers[regIntStatus0];
            //ret = 0xff;
            //registers[regIntStatus0] = 0;
            bitSet(registers[regIntStatus0], flagEND);
            //debug_log(DEBUG_DETAIL, "[GPIB] status reg: 0x%02X\n", registers[regIntStatus0]);
            update_int();
            break;
          case regIntStatus1:
            ret = registers[regIntStatus1];
            //ret = 0xff;
            registers[regIntStatus1] = 0;
            update_int();
            break;
          case regDataOut:
          /*
            ret = m_reg_di;
            BIT_CLR(m_reg_int0_status , REG_INT0_BI_BIT);
            update_int();
            set_accrq(false);
            if (!m_hdfa && m_ah_anhs) {
              m_ah_anhs = false;
              update_fsm();
            }
            // TODO: ACRS -> ANRS ?
          */
          default:
            ret = 0;
        }

        //return registers[addr];
        //return ret;

}


void tms9914a_write(void* dummy, uint32_t addr, uint8_t value) {
        addr = addr - baseAddress;
        addr = addr >> 1;
#ifdef DEBUG_GPIB
            debug_log(DEBUG_DETAIL, "[GPIB] Write addr: 0x%02X\t0x%02X\n", addr, value);
#endif

        addr += 8;
        
        switch(addr) {
          case regIntMask0:
#ifdef DEBUG_GPIB
            debug_log(DEBUG_DETAIL, "[GPIB] Write regIntMask0: 0x%02X\n", value);
#endif
            break;
          case regIntMask1:
#ifdef DEBUG_GPIB
            debug_log(DEBUG_DETAIL, "[GPIB] Write regIntMask1: 0x%02X\n", value);
#endif
            break;
          case regAuxCmd:
            //doAuxCmd(value);
            do_aux_cmd(value & auxCmdMask, (value & auxBitMask) >> 7);
            break;
          case regDataOut:
#ifdef DEBUG_GPIB
            debug_log(DEBUG_DETAIL, "[GPIB] Write regDataOut: 0x%02X\n", value);
#endif
            break;
          default:
#ifdef DEBUG_GPIB
            debug_log(DEBUG_DETAIL, "[GPIB] Write port 0x%02X: 0x%02X\n", addr - 8, value);
#endif
            break;
        }
}


void busUpdate() {
  
}

void tms9914a_init() {
#ifdef DEBUG_GPIB
        debug_log(DEBUG_INFO, "[GPIB] Initializing GPiB controller\r\n");
#endif
        registers[regSerialPool] = 0;
        registers[regParallellPool] = 0;
        auxBits = 0;
        bitSet(auxBits, flagSWRST);
        do_aux_cmd(AUXCMD_SWRST, 1);
#ifdef DEBUG_GPIB
        debug_log(DEBUG_INFO, "[GPIB] Hardware reset during init\n");
        debug_log(DEBUG_INFO, "[GPIB] Map callback:\tbase\t0x%05X\tlen:\t0x%02X\r\n", baseAddress, addressLen);
#endif

        memory_mapCallbackRegister(baseAddress, addressLen, (void*)tms9914a_read, (void*)tms9914a_write, NULL);
        busMemberId = GPiBregisterClient(busUpdate, "GRiD");
        //if (busMemberId < 0) return -1;
}

void update_int() {
	bool new_int_line = false;
	registers[regIntStatus0] &= 0b11111100;
	if (registers[regIntStatus0] & registers[regIntMask0]) {
		bitSet(registers[regIntStatus0] , flagINT0);
		new_int_line = true;
	}
	if (registers[regIntStatus1] & registers[regIntMask1]) {
		bitSet(registers[regIntStatus0] , flagINT1);
		new_int_line = true;
	}
    
  
	if ( bitRead(auxBits, flagDAI) ) {
		new_int_line = false;
	}
  

	if (new_int_line != irqLine) {
#ifdef DEBUG_GPIB
    debug_log(DEBUG_DETAIL, "[GPIB] call Interrupt 2\n");
#endif
		//LOG_INT("INT=%d\n" , new_int_line);
		irqLine = new_int_line;
    //rise IRQ!
		//m_int_write_func(m_int_line);
    //i8259_doirq(irqGPiB);
    i8259_setirq(irqGPiB, true);
	}

}

bool controller_reset() {
  return bitRead(auxBits, flagSWRST) || get_ifcin();
	//return m_swrst || get_ifcin();
}

bool get_ifcin() {
	return GPiBbusIFCget() && !bitRead(auxBits, flagSIC);
  //return get_signal(IEEE_488_IFC) && !m_sic;
}

void do_swrst() {
  
}

void update_fsm() {
	if (reflection) {
		return;
	}

  reflection = true;
  
  bool changed = true;
	//uint8_t prev_state;
  while (changed) {
    changed = externalStateChange;
    externalStateChange = false;
  }
}
