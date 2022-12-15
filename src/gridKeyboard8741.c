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

#include <stdbool.h>
#include <stdint.h>
#include <string.h>
#include "config.h"
#include "debuglog.h"
#include "gridKeyboard8741.h"
#include "keymap.h"
#include "memory.h"
#include "machine.h"
#include "sdlconsole.h"
#include "utility.h"

uint32_t scanCode = 0;
uint8_t keybReady = true;
uint8_t gotSDLcode = false;
#define baseAddress 0xDFFC0
#define addressLen  0x3

I8259_t* irq8259;

uint8_t translateScancode(uint32_t keyval, uint8_t modKeys) {
  uint8_t ret;
  modKeys = 0x7F & modKeys;
  //keyval = keyval & 0xff;
  
	for (uint8_t i = 0; i < sizeof(keyTranslateMatrix)/sizeof(keyTranslateMatrix[0]); i++) {
		//if (keyval == (SDL_Keycode)keyTranslateMatrix[i][0]) {
                if (keyval == (SDL_Keycode)keyTranslateMatrix[i].sdlKeyName) {
			//return keyTranslateMatrix[i][1];
                    
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
  //scanCode = lScanCode;
  if ( bitRead(lModKeys,7)) {
    scanCode = translateScancode(lScanCode, lModKeys);
  } else {
    scanCode = 0xff;
  }
  gotSDLcode = true;
  i8259_doirq(irq8259, 4);
  debug_log(DEBUG_DETAIL, "[KEY] getScanCode: 0x%02X\tSDL code: 0x%08X\tmodkeys: 0x%02X\n", scanCode, lScanCode, lModKeys);
} 
uint8_t gridKeyboard8741_read(void* dummy, uint32_t addr) {
  addr = addr - baseAddress;
  addr = addr >> 1;
	//scanCode = sdlconsole_getScancode();
  //scanCode = machine.KeyState.scancode;
  uint8_t ret;
#ifdef DEBUG_KEYBOARD
  debug_log(DEBUG_DETAIL, "[KEY] Read port 0x%02X\n", addr);
#endif
  //return 0x00;
  
  switch (addr) {
    case 0:
#ifdef DEBUG_KEYBOARD
      debug_log(DEBUG_DETAIL, "[KEY] scancode 0x%02X\n", scanCode);
#endif

      //scanCode = 0x00;
      if (gotSDLcode) {
        ret = scanCode;
        scanCode = 0xff;
        gotSDLcode = false;
        keybReady = false;
      } //else {
        //keybReady = true;
      //}
      
      return ret;
      break;
    case 1:
      if (keybReady) {
        return 0;
      } else {
        keybReady = true;
        return 2;
      }
      //return keybReady ? 2 : 0;
  }
	//return 0;
        //return 0x0;
}

void gridKeyboard8741_write(void* dummy, uint32_t addr, uint8_t value) {
  addr = addr - baseAddress;
  addr = addr >> 1;
#ifdef DEBUG_KEYBOARD
  debug_log(DEBUG_DETAIL, "[KEY] Write port 0x%02X: %02X\n", addr, value);
#endif
  keybReady = false;
}

void gridKeyboard8741_init(I8259_t* i8259) {
#ifdef DEBUG_KEYBOARD
        debug_log(DEBUG_INFO, "[KEY] Attaching to memory\r\n");
#endif
        memory_mapCallbackRegister(0xDFFC0, 0x4, (void*)gridKeyboard8741_read, (void*)gridKeyboard8741_write, NULL);
        irq8259 = i8259;
}
