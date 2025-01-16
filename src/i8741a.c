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

#include <stdbool.h>
#include <stdint.h>
#include <stdbool.h>
#include <string.h>
#include "config.h"
#include "debuglog.h"
#include "i8741a.h"
#include "keymap.h"
#include "memory.h"
#include "machine.h"
#include "sdlconsole.h"
#include "utility.h"
#include "timing.h"


uint32_t scanCode = 0;
uint8_t keybReady = true;
uint8_t gotSDLcode = false;
#define baseAddress 0xDFFC0
#define addressLen  0x3

I8259_t* keyIrq8259;

uint8_t busData, busStatus;

uint8_t configRegister[4];
uint8_t configRegisterPointer = 0;
uint8_t wdt1;
uint8_t globalState = 0;
bool timer_overflow = false;

void sendData(uint8_t data, uint8_t flags) {
  busData = data;
  busStatus = flags;
  i8259_doirq(keyIrq8259, 4);
}

uint8_t translateScancode(uint32_t keyval, uint8_t modKeys) {
  uint8_t ret;
  modKeys = 0x7F & modKeys;
  
  for (uint8_t i = 0; i < sizeof(keyTranslateMatrix)/sizeof(keyTranslateMatrix[0]); i++) {
    if (keyval == (SDL_Keycode)keyTranslateMatrix[i].sdlKeyName) {
      switch (modKeys) {
        case 0x0:
          ret = keyTranslateMatrix[i].unshifted;
          break;
        case 0x10:
          ret = keyTranslateMatrix[i].shift;
          break;
        case 0x20:
          ret = keyTranslateMatrix[i].code;
          break;
        case 0x30:
          ret = keyTranslateMatrix[i].codeShift;
          break;
        case 0x40:
          ret = keyTranslateMatrix[i].ctrl;
          break;
        case 0x50:
          ret = keyTranslateMatrix[i].shiftCtrl;
          break;
        case 0x60:
          ret = keyTranslateMatrix[i].codeCtrl;
          break;
        case 0x70:
          ret = keyTranslateMatrix[i].codeShiftCtr;
          break;

        default:
          ret = keyTranslateMatrix[i].unshifted;
          break;
      }
    return ret;
    }
	}
  debug_log(DEBUG_DETAIL, "keyval: 0x%08X not found\n", keyval);
	return 0x00;
}

void gridKeyboard8741_getScanCode(uint32_t lScanCode, uint8_t lModKeys) {
  if ( bitRead(lModKeys,7)) {
    scanCode = translateScancode(lScanCode, lModKeys);
  } else {
    scanCode = 0xff;
  }
  gotSDLcode = true;
  debug_log(DEBUG_DETAIL, "[KEY] getScanCode: 0x%02X\tSDL code: 0x%08X\tmodkeys: 0x%02X\n", scanCode, lScanCode, lModKeys);
} 

uint8_t gridKeyboard8741_read(void* dummy, uint32_t addr) {
  addr = addr - baseAddress;
  addr = addr >> 1;

#ifdef DEBUG_KEYBOARD
  debug_log(DEBUG_DETAIL, "[KEY] Read %s port\n", addr ? "Status" : "Data");
#endif
  
  if (addr == 0) {    // CPU reads data port
#ifdef DEBUG_KEYBOARD
      debug_log(DEBUG_DETAIL, "[KEY] read busData: 0x%02X\n", busData);
#endif
      return busData;
  } else if (addr == 1) {
#ifdef DEBUG_KEYBOARD
      debug_log(DEBUG_DETAIL, "[KEY] read busStatus: 0x%02X\n", busStatus);
#endif
      return busStatus;
  } else {
    debug_log(DEBUG_DETAIL, "[KEY] address error: 0x%04X\n", addr);
    return 0x00;
  }

}

void gridKeyboard8741_write(void* dummy, uint32_t addr, uint8_t value) {
  addr = addr - baseAddress;
  addr = addr >> 1;
#ifdef DEBUG_KEYBOARD
  debug_log(DEBUG_DETAIL, "[KEY] Write %s port: 0x%02X\n", addr ? "Command" : "Data", value);

  //debug_log(DEBUG_DETAIL, "[cpu] exec: Addr: %04X:%04X, opcode: %02X\r\n", machine.CPU.segregs[regcs], machine.CPU.ip, machine.CPU.opcode);
  //debug_log(DEBUG_DETAIL, "[cpu] regs: AX: %04X, BX: %04X, CX: %04X, DX: %04X\r\n", machine.CPU.regs.wordregs[regax], machine.CPU.regs.wordregs[regbx], machine.CPU.regs.wordregs[regcx], machine.CPU.regs.wordregs[regdx]);
  //debug_log(DEBUG_DETAIL, "[cpu] regs: SI: %04X, DI: %04X, BP: %04X, SP: %04X\r\n", machine.CPU.regs.wordregs[regsi], machine.CPU.regs.wordregs[regdi], machine.CPU.regs.wordregs[regbp], machine.CPU.regs.wordregs[regsp]);
  //debug_log(DEBUG_DETAIL, "[cpu] regs: CS: %04X, DS: %04X, ES: %04X, SS: %04X\r\n", machine.CPU.segregs[regcs], machine.CPU.segregs[regds], machine.CPU.segregs[reges], machine.CPU.segregs[regss]);

#endif

  // data mode
  if (addr == 0) {
    configRegister[configRegisterPointer] = value;
    return;
  }
  
  // command mode
  if ( bitRead(value, 7) == 0 ) {
    globalState = value;
#ifdef DEBUG_KEYBOARD
    debug_log(DEBUG_DETAIL, "[KEY] global state 0x%02X\n", value);
    debug_log(DEBUG_DETAIL, "[KEY] Key scan state state: %d\n", bitRead(value, 0) );
    debug_log(DEBUG_DETAIL, "[KEY] WDT1 state: %d\n", bitRead(value, 1) );
    debug_log(DEBUG_DETAIL, "[KEY] WDT0 state: %d\n", bitRead(value, 2) );
    debug_log(DEBUG_DETAIL, "[KEY] Long routine state/repeat request: %d\n", bitRead(value, 3) );
    debug_log(DEBUG_DETAIL, "[KEY] pinPAL state: %d\n", bitRead(value, 4) );
#endif
    return;
  }
  
  if (bitRead(value, 6) == 0) {
    configRegisterPointer = value & 0x3;
    debug_log(DEBUG_DETAIL, "[KEY] config reg selected 0x%02X\n", configRegisterPointer);
    return;
  }
  
  wdt1 = configRegister[3];
  debug_log(DEBUG_DETAIL, "[KEY] kicking WDT1\n");

}

void gridKeyboard8741_tickCallback(void* dummy) {
  static int timerOverflowCount = 0;
  
  timerOverflowCount--;

  if (timerOverflowCount  <= 0) {
    timerOverflowCount = 2;

    if (bitRead(globalState, 1) == 1) {
      wdt1--;
      if ( wdt1 == 0 ) {
      // set pinWTF
        sendData(0xFD, 0);
      }
    }
    
    if (gotSDLcode == true && bitRead(globalState, 0) == 1) {
      sendData(scanCode, 0);
      scanCode = 0xff;
      gotSDLcode = false;
      keybReady = false;     
    }
  }

}

void gridKeyboard8741_init(I8259_t* i8259) {
#ifdef DEBUG_KEYBOARD
        debug_log(DEBUG_INFO, "[KEY] Attaching to memory\r\n");
#endif
        memory_mapCallbackRegister(0xDFFC0, 0x4, (void*)gridKeyboard8741_read, (void*)gridKeyboard8741_write, NULL);
        keyIrq8259 = i8259;
        
        configRegister[0] = 0x01;
        configRegister[1] = 0x0F;
        configRegister[2] = 0x02;
        configRegister[3] = 0x32;
        
        timing_addTimer(gridKeyboard8741_tickCallback, (void*) NULL, 1500, TIMING_ENABLED);
}
