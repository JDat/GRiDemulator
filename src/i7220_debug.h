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

#ifndef _BUBBLE_DEBUG_H_
#define _BUBBLE_DEBUG_H_

#include <stdint.h>

  #ifdef DEBUG_BUBBLEMEM
      static const char *regNamesChar[] = {
          "Utility register",
          "Block length register LSB",
          "Block length register MSB",
          "Enable register",
          "Address register LSB",
          "Address register MSB",
      };

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

#endif
