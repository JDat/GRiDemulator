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

#ifndef _CPU_H_
#define _CPU_H_

#include <stdint.h>
#include <stdbool.h>
#include "config.h"
#include "utility.h"
#include "i8259.h"

union _bytewordregs_ {
  uint16_t wordregs[8];
  uint8_t byteregs[8];
};

typedef struct {
  union _bytewordregs_ regs;
  uint8_t opcode, segoverride, reptype, hltstate;
  uint16_t segregs[4], ip, useseg, oldsp;
  uint8_t tempcf, oldcf, cf, pf, af, zf, sf, tf, ifl, df, of, mode, reg, rm;
  uint16_t oper1, oper2, res16, disp16, temp16, dummy, stacksize, frametemp;
  uint8_t oper1b, oper2b, res8, disp8, temp8, nestlev, addrbyte;
  uint32_t temp1, temp2, temp3, temp4, temp5, temp32, tempaddr32, ea;
  int32_t result;
  uint16_t trap_toggle;
  uint16_t savecs, saveip;
  uint8_t usedRegister;
  //uint64_t totalexec;
  //void (*int_callback[256])(void*, uint8_t); //Want to pass a CPU object in first param, but it's not defined at this point so use a void*
} CPU_t;

#define regax 0
#define regcx 1
#define regdx 2
#define regbx 3
#define regsp 4
#define regbp 5
#define regsi 6
#define regdi 7

#define reges 0
#define regcs 1
#define regss 2
#define regds 3

#ifdef __BIG_ENDIAN__
#define regal 1
#define regah 0
#define regcl 3
#define regch 2
#define regdl 5
#define regdh 4
#define regbl 7
#define regbh 6
#else
#define regal 0
#define regah 1
#define regcl 2
#define regch 3
#define regdl 4
#define regdh 5
#define regbl 6
#define regbh 7
#endif

#define segbase(x)  ((uint32_t) x << 4)
#define signext(value)  (int16_t)(int8_t)(value)
#define signext32(value)  (int32_t)(int16_t)(value)

#define getmem8(mycpu, x, y)  cpu_read(mycpu, segbase(x) + y)
#define getmem16(mycpu, x, y) cpu_readw(mycpu, segbase(x) + y)
#define putmem8(mycpu, x, y, z) cpu_write(mycpu, segbase(x) + y, z)
#define putmem16(mycpu, x, y, z)  cpu_writew(mycpu, segbase(x) + y, z)

#define getreg16(mycpu, regid)  mycpu->regs.wordregs[regid]
#define getreg8(mycpu, regid) mycpu->regs.byteregs[byteregtable[regid]]
#define putreg16(mycpu, regid, writeval)  mycpu->regs.wordregs[regid] = writeval
#define putreg8(mycpu, regid, writeval) mycpu->regs.byteregs[byteregtable[regid]] = writeval

#define getsegreg(mycpu, regid) mycpu->segregs[regid]
#define putsegreg(mycpu, regid, writeval) mycpu->segregs[regid] = writeval

#define makeflagsword(x) \
  ( \
  2 | (uint16_t) x->cf | ((uint16_t) x->pf << 2) | ((uint16_t) x->af << 4) | ((uint16_t) x->zf << 6) | ((uint16_t) x->sf << 7) | \
  ((uint16_t) x->tf << 8) | ((uint16_t) x->ifl << 9) | ((uint16_t) x->df << 10) | ((uint16_t) x->of << 11) \
  )

#define decodeflagsword(x,y) { \
  uint16_t tmp; \
  tmp = y; \
  x->cf = tmp & 1; \
  x->pf = (tmp >> 2) & 1; \
  x->af = (tmp >> 4) & 1; \
  x->zf = (tmp >> 6) & 1; \
  x->sf = (tmp >> 7) & 1; \
  x->tf = (tmp >> 8) & 1; \
  x->ifl = (tmp >> 9) & 1; \
  x->df = (tmp >> 10) & 1; \
  x->of = (tmp >> 11) & 1; \
}

/*
const t_8087command opcodesMatrix8087[64] = {
        { NULL },
        { NULL }
        };
*/
        
void do8087(CPU_t* cpu);
void StepIP(CPU_t* cpu, uint8_t x);
void modregrm(CPU_t* x);

uint8_t cpu_read(CPU_t* cpu, uint32_t addr);
uint16_t cpu_readw(CPU_t* cpu, uint32_t addr);
void cpu_write(CPU_t* cpu, uint32_t addr32, uint8_t value);
void cpu_writew(CPU_t* cpu, uint32_t addr32, uint16_t value);

void cpu_intcall(CPU_t* cpu, uint8_t intnum);
void cpu_reset(CPU_t* cpu);
void cpu_interruptCheck(CPU_t* cpu);

// Check interrupt etc stuff and execute one instruction
// Return how many cycles used to execure instruction
uint32_t cpu_exec(CPU_t* cpu);

void port_write(CPU_t* cpu, uint16_t portnum, uint8_t value);
void port_writew(CPU_t* cpu, uint16_t portnum, uint16_t value);
uint8_t port_read(CPU_t* cpu, uint16_t portnum);
uint16_t port_readw(CPU_t* cpu, uint16_t portnum);

//void cpu_registerIntCallback(CPU_t* cpu, uint8_t interrupt, void (*cb)(CPU_t*, uint8_t));

#endif
