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

#ifndef _GRiDkey8741_H_
#define _GRiDkey8741_H_

#include <stdint.h>
#include "input.h"

void gridKeyboard8741_getScanCode(uint8_t lScanCode);
void gridKeyboard8741_init();
void gridKeyboard8741_doirq(uint8_t irqnum);

void gridKeyboard8741_write(void* dummy, uint32_t addr, uint8_t value);
uint8_t gridKeyboard8741_read(void* dummy, uint32_t addr);

#endif