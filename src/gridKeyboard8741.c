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
	Intel 8259 interrupt controller
*/

#include <stdbool.h>
#include <stdint.h>
#include <string.h>
#include "config.h"
#include "debuglog.h"
#include "gridKeyboard8741.h"
//#include "../ports.h"
#include "memory.h"
#include "machine.h"
//#include "input.h"
#include "sdlconsole.h"

uint8_t scanCode;
uint8_t keybReady;

void gridKeyboard8741_getScanCode(uint8_t lScanCode) {
  scanCode = lScanCode;
  keybReady = true;
} 
uint8_t gridKeyboard8741_read(void* dummy, uint32_t addr) {
  addr = addr - 0xDFFC0;
	//scanCode = sdlconsole_getScancode();
  //scanCode = machine.KeyState.scancode;
  uint8_t ret;
#ifdef DEBUG_KEYBOARD
  debug_log(DEBUG_DETAIL, "[KEY] Read port 0x%02X\n", addr);
#endif
  switch (addr) {
    case 0:
#ifdef DEBUG_KEYBOARD
      debug_log(DEBUG_DETAIL, "[KEY] scancode 0x%02X\n", scanCode);
#endif
      keybReady = false;
      ret = scanCode;
      scanCode = 0xFF;
      return ret;
      break;
    case 2:
      return keybReady ? 2 : 0;
  }
	//return 0;
        return 0x0;
}

void gridKeyboard8741_write(void* dummy, uint32_t addr, uint8_t value) {
	addr = addr - 0xDFFC0;
#ifdef DEBUG_KEYBOARD
  debug_log(DEBUG_DETAIL, "[KEY] Write port 0x%02X: %02X\n", addr, value);
#endif
  keybReady = false;
}

void gridKeyboard8741_init() {
#ifdef DEBUG_KEYBOARD
        debug_log(DEBUG_INFO, "[KEY] Attaching to memory\r\n");
#endif
        memory_mapCallbackRegister(0xDFFC0, 0x4, (void*)gridKeyboard8741_read, (void*)gridKeyboard8741_write, NULL);
}
