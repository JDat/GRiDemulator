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

#ifndef _PORTS_H_
  #define _PORTS_H_

  #include <stdint.h>

  extern uint8_t(*ports_cbReadB[PORTS_COUNT])(uint32_t portnum);
  //extern uint16_t(*ports_cbReadW[PORTS_COUNT])(uint32_t portnum);
  extern void (*ports_cbWriteB[PORTS_COUNT])(uint32_t portnum, uint8_t value);
  //extern void (*ports_cbWriteW[PORTS_COUNT])(uint32_t portnum, uint16_t value);

  //void ports_cbRegister(uint32_t start, uint32_t count, uint8_t(*readb)(uint32_t), uint16_t(*readw)(uint32_t), void (*writeb)(uint32_t, uint8_t), void (*writew)(uint32_t, uint16_t));
  void ports_cbRegister(uint32_t start, uint32_t count, uint8_t(*readb)(uint32_t), void (*writeb)(uint32_t, uint8_t));
  void ports_init();

#endif // _PORTS_H_
