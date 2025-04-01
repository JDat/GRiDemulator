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

#ifndef _I8259_H_
    #define _I8259_H_

    #include <stdbool.h>
    typedef struct {
        uint8_t imr; //mask register
        uint8_t irr; //request register
        uint8_t isr; //service register
        uint8_t icwstep; //used during initialization to keep track of which ICW we're at
        uint8_t icw[5];
        uint8_t ocw[5];
        uint8_t readmode; //remember what to return on read register from OCW3
        uint8_t lock;       //semaphore. locked while CPU processing IRQ
    } I8259_t;

    void i8259_init();
    void i8259_doirq(uint8_t irqnum);
    void i8259_setirq(uint8_t irqnum, uint8_t state);
    uint8_t i8259_nextintr();
    bool i8259_haveInt();
    void i8259_write(uint16_t portnum, uint8_t value);
    uint8_t i8259_read(uint16_t portnum);

#endif //_I8259_H_
