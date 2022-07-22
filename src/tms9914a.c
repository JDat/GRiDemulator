/*
  XTulator: A portable, open-source 80186 PC emulator.
  Copyright (C)2020 Mike Chambers

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
	Bubble memory
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
#include "GPIB_bus.h"
//#include <time.h>

#define baseAddress 0xDFF80
#define addressLen  0xF

// Auxiliary commands

uint8_t registers[16];
uint32_t auxBits;
bool irqLine;

I8259_t* i8259;

void do_aux_cmd(unsigned cmd , bool set_bit) {

  switch(cmd) {
    case AUXCMD_SWRST:
      //fixme
      break;
    case AUXCMD_SIC:
    if ( bitRead(auxBits, flagSIC) != set_bit ) {
        bitWrite(auxBits, flagSIC, set_bit);
        //update ifc
        //set_signal(IEEE_488_IFC , m_sic);
    }
    /*
      if (m_sic != set_bit) {
        m_sic = set_bit;
        update_ifc();
        if (!controller_reset() && m_sic && m_c_state == FSM_C_CIDS) {
          m_c_state = FSM_C_CADS;
          m_ext_state_change = true;
        }
        update_fsm();
      }
      */
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
        switch(addr) {
          case regIntStatus0:
            //ret = registers[regIntStatus0];
            ret = 0xff;
            registers[regIntStatus0] = 0;
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
        return ret;

}


void tms9914a_write(void* dummy, uint32_t addr, uint8_t value) {
        addr = addr - baseAddress;
        addr = addr >> 1;
        addr += 8;
        switch(addr) {
          case regAuxCmd:
            //doAuxCmd(value);
            do_aux_cmd(value & auxCmdMask, (value & auxBitMask) >> 7);
            break;
          default:
#ifdef DEBUG_GPIB
            debug_log(DEBUG_DETAIL, "[GPIB] Write port 0x%02X: 0x%02X\n", addr, value);
#endif
            break;
        }
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
        memory_mapCallbackRegister(baseAddress, addressLen, (void*)tms9914a_read, (void*)tms9914a_write, NULL);
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
    debug_log(DEBUG_DETAIL, "[GPIB] call Interrupt 5\n");
#endif
		//LOG_INT("INT=%d\n" , new_int_line);
		irqLine = new_int_line;
    //rise IRQ!
		//m_int_write_func(m_int_line);
    i8259_doirq(i8259, 5);
	}

}
