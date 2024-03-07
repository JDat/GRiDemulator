/*
  GRiD Compass emulator
  Copyright (C)2022 JDat
  https://github.com/JDat/GRiDemulator

  Based on XTulator: A portable, open-source 80186 PC emulator.
  Copyright (C)2020 Mike Chambers
  https://github.com/mikechambers84/XTulator

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

#ifndef _RTC_H_
#define _RTC_H_

#include <stdint.h>
//#include "cpu.h"

uint8_t rtc_read(void* dummy, uint32_t addr);
void rtc_write(void* dummy, uint32_t addr, uint8_t value);

//uint8_t rtc_read(void* dummy, uint16_t addr);
//void rtc_write(void* dummy, uint16_t addr, uint8_t value);
void rtc_init();

#endif
