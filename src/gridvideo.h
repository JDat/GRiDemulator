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

#ifndef _GRIDVIDEO_H_
  #define _GRIDVIDEO_H_

  #include <stdint.h>

  int gridvideo_init();

  void gridvideo_update();    // called by SDL from main() to update GUI

  void gridvideo_scanlineCallback(void* dummy);   // rise IRQ on every scanline

  // CPU videoRAM interface
  void gridvideo_writememory(void* dummy, uint32_t addr, uint8_t value);
  uint8_t gridvideo_readmemory(void* dummy, uint32_t addr);

  //void gridvideo_drawCallback(void* dummy);   // do we need this?

#endif  // _GRIDVIDEO_H_
