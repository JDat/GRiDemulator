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

#include <stdint.h>
#include <stddef.h>
#include <stdbool.h>
#include "cpu.h"
#include "config.h"
#include "debuglog.h"
#include "memory.h"


#ifdef DEBUG_CPU
  uint16_t prevSeg=0xFFFF;
#endif

#ifdef DEBUG_DISASM
static const char *cpuRegNamesChar[] = {
  "ax",
  "cx",
  "dx",
  "bx",
  "sp",
  "bp",
  "si",
  "di",
  "es",
  "cs",
  "ss",
  "ds"
};
/*
static const char *cpuRegNames8Char[] = {
  "ah",
  "al",
  "bh",
  "bl",
  "ch",
  "cl",
  "dh",
  "dl",
};
*/
#endif

const uint8_t byteregtable[8] = { regal, regcl, regdl, regbl, regah, regch, regdh, regbh };

const uint8_t parity[0x100] = {
  1, 0, 0, 1, 0, 1, 1, 0, 0, 1, 1, 0, 1, 0, 0, 1, 0, 1, 1, 0, 1, 0, 0, 1, 1, 0, 0, 1, 0, 1, 1, 0,
  0, 1, 1, 0, 1, 0, 0, 1, 1, 0, 0, 1, 0, 1, 1, 0, 1, 0, 0, 1, 0, 1, 1, 0, 0, 1, 1, 0, 1, 0, 0, 1,
  0, 1, 1, 0, 1, 0, 0, 1, 1, 0, 0, 1, 0, 1, 1, 0, 1, 0, 0, 1, 0, 1, 1, 0, 0, 1, 1, 0, 1, 0, 0, 1,
  1, 0, 0, 1, 0, 1, 1, 0, 0, 1, 1, 0, 1, 0, 0, 1, 0, 1, 1, 0, 1, 0, 0, 1, 1, 0, 0, 1, 0, 1, 1, 0,
  0, 1, 1, 0, 1, 0, 0, 1, 1, 0, 0, 1, 0, 1, 1, 0, 1, 0, 0, 1, 0, 1, 1, 0, 0, 1, 1, 0, 1, 0, 0, 1,
  1, 0, 0, 1, 0, 1, 1, 0, 0, 1, 1, 0, 1, 0, 0, 1, 0, 1, 1, 0, 1, 0, 0, 1, 1, 0, 0, 1, 0, 1, 1, 0,
  1, 0, 0, 1, 0, 1, 1, 0, 0, 1, 1, 0, 1, 0, 0, 1, 0, 1, 1, 0, 1, 0, 0, 1, 1, 0, 0, 1, 0, 1, 1, 0,
  0, 1, 1, 0, 1, 0, 0, 1, 1, 0, 0, 1, 0, 1, 1, 0, 1, 0, 0, 1, 0, 1, 1, 0, 0, 1, 1, 0, 1, 0, 0, 1
};

void StepIP(CPU_t* cpu, uint8_t x) {
  cpu->ip += x;
}

void modregrm(CPU_t* x) {
  x->addrbyte = getmem8(x, x->segregs[regcs], x->ip);
  StepIP(x, 1);
  x->mode = x->addrbyte >> 6;
  x->reg = (x->addrbyte >> 3) & 7;
  x->rm = x->addrbyte & 7;
  switch(x->mode) {
    case 0:
      if(x->rm == 6) {
        x->disp16 = getmem16(x, x->segregs[regcs], x->ip);
        StepIP(x, 2);
      }
      if(((x->rm == 2) || (x->rm == 3)) && !x->segoverride) {
        x->useseg = x->segregs[regss];
      }
      break;

    case 1:
      x->disp16 = signext(getmem8(x, x->segregs[regcs], x->ip));
      StepIP(x, 1);
      if(((x->rm == 2) || (x->rm == 3) || (x->rm == 6)) && !x->segoverride) {
        x->useseg = x->segregs[regss];
      }
      break;

    case 2:
      x->disp16 = getmem16(x, x->segregs[regcs], x->ip);
      StepIP(x, 2);
      if(((x->rm == 2) || (x->rm == 3) || (x->rm == 6)) && !x->segoverride) {
        x->useseg = x->segregs[regss];
      }
      break;

    default:
      x->disp8 = 0;
      x->disp16 = 0;
  }
}

void cpu_writew(CPU_t* cpu, uint32_t addr32, uint16_t value) {
  cpu_write(cpu, addr32, (uint8_t)value);
  cpu_write(cpu, addr32 + 1, (uint8_t)(value >> 8));
}

uint16_t cpu_readw(CPU_t* cpu, uint32_t addr32) {
  return ((uint16_t)cpu_read(cpu, addr32) | (uint16_t)(cpu_read(cpu, addr32 + 1) << 8));
}

void flag_szp8(CPU_t* cpu, uint8_t value) {
  if (!value) {
    cpu->zf = 1;
  } else {
    cpu->zf = 0;    /* set or clear zero flag */
  }

  if (value & 0x80) {
    cpu->sf = 1;
  } else {
    cpu->sf = 0;    /* set or clear sign flag */
  }

  cpu->pf = parity[value]; /* retrieve parity state from lookup table */
}

void flag_szp16(CPU_t* cpu, uint16_t value) {
  if (!value) {
    cpu->zf = 1;
  } else {
    cpu->zf = 0;    /* set or clear zero flag */
  }

  if (value & 0x8000) {
    cpu->sf = 1;
  } else {
    cpu->sf = 0;    /* set or clear sign flag */
  }

  cpu->pf = parity[value & 255];  /* retrieve parity state from lookup table */
}

void flag_log8(CPU_t* cpu, uint8_t value) {
  flag_szp8(cpu, value);
  cpu->cf = 0;
  cpu->of = 0; /* bitwise logic ops always clear carry and overflow */
}

void flag_log16(CPU_t* cpu, uint16_t value) {
  flag_szp16(cpu, value);
  cpu->cf = 0;
  cpu->of = 0; /* bitwise logic ops always clear carry and overflow */
}

void flag_adc8(CPU_t* cpu, uint8_t v1, uint8_t v2, uint8_t v3) {
  /* v1 = destination operand, v2 = source operand, v3 = carry flag */
  uint16_t        dst;

  dst = (uint16_t)v1 + (uint16_t)v2 + (uint16_t)v3;
  flag_szp8(cpu, (uint8_t)dst);
  if (((dst ^ v1) & (dst ^ v2) & 0x80) == 0x80) {
    cpu->of = 1;
  } else {
    cpu->of = 0; /* set or clear overflow flag */
  }

  if (dst & 0xFF00) {
    cpu->cf = 1;
  } else {
    cpu->cf = 0; /* set or clear carry flag */
  }

  if (((v1 ^ v2 ^ dst) & 0x10) == 0x10) {
    cpu->af = 1;
  } else {
    cpu->af = 0; /* set or clear auxilliary flag */
  }
}

void flag_adc16(CPU_t* cpu, uint16_t v1, uint16_t v2, uint16_t v3) {

  uint32_t        dst;

  dst = (uint32_t)v1 + (uint32_t)v2 + (uint32_t)v3;
  flag_szp16(cpu, (uint16_t)dst);
  if ((((dst ^ v1) & (dst ^ v2)) & 0x8000) == 0x8000) {
    cpu->of = 1;
  } else {
    cpu->of = 0;
  }

  if (dst & 0xFFFF0000) {
    cpu->cf = 1;
  } else {
    cpu->cf = 0;
  }

  if (((v1 ^ v2 ^ dst) & 0x10) == 0x10) {
    cpu->af = 1;
  } else {
    cpu->af = 0;
  }
}

void flag_add8(CPU_t* cpu, uint8_t v1, uint8_t v2) {
  /* v1 = destination operand, v2 = source operand */
  uint16_t        dst;

  dst = (uint16_t)v1 + (uint16_t)v2;
  flag_szp8(cpu, (uint8_t)dst);
  if (dst & 0xFF00) {
    cpu->cf = 1;
  } else {
    cpu->cf = 0;
  }

  if (((dst ^ v1) & (dst ^ v2) & 0x80) == 0x80) {
    cpu->of = 1;
  } else {
    cpu->of = 0;
  }

  if (((v1 ^ v2 ^ dst) & 0x10) == 0x10) {
    cpu->af = 1;
  } else {
    cpu->af = 0;
  }
}

void flag_add16(CPU_t* cpu, uint16_t v1, uint16_t v2) {
  /* v1 = destination operand, v2 = source operand */
  uint32_t        dst;

  dst = (uint32_t)v1 + (uint32_t)v2;
  flag_szp16(cpu, (uint16_t)dst);
  if (dst & 0xFFFF0000) {
    cpu->cf = 1;
  } else {
    cpu->cf = 0;
  }

  if (((dst ^ v1) & (dst ^ v2) & 0x8000) == 0x8000) {
    cpu->of = 1;
  } else {
    cpu->of = 0;
  }

  if (((v1 ^ v2 ^ dst) & 0x10) == 0x10) {
    cpu->af = 1;
  } else {
    cpu->af = 0;
  }
}

void flag_sbb8(CPU_t* cpu, uint8_t v1, uint8_t v2, uint8_t v3) {

  /* v1 = destination operand, v2 = source operand, v3 = carry flag */
  uint16_t        dst;

  v2 += v3;
  dst = (uint16_t)v1 - (uint16_t)v2;
  flag_szp8(cpu, (uint8_t)dst);
  if (dst & 0xFF00) {
    cpu->cf = 1;
  } else {
    cpu->cf = 0;
  }

  if ((dst ^ v1) & (v1 ^ v2) & 0x80) {
    cpu->of = 1;
  } else {
    cpu->of = 0;
  }

  if ((v1 ^ v2 ^ dst) & 0x10) {
    cpu->af = 1;
  } else {
    cpu->af = 0;
  }
}

void flag_sbb16(CPU_t* cpu, uint16_t v1, uint16_t v2, uint16_t v3) {

  /* v1 = destination operand, v2 = source operand, v3 = carry flag */
  uint32_t        dst;

  v2 += v3;
  dst = (uint32_t)v1 - (uint32_t)v2;
  flag_szp16(cpu, (uint16_t)dst);
  if (dst & 0xFFFF0000) {
    cpu->cf = 1;
  } else {
    cpu->cf = 0;
  }

  if ((dst ^ v1) & (v1 ^ v2) & 0x8000) {
    cpu->of = 1;
  } else {
    cpu->of = 0;
  }

  if ((v1 ^ v2 ^ dst) & 0x10) {
    cpu->af = 1;
  } else {
    cpu->af = 0;
  }
}

void flag_sub8(CPU_t* cpu, uint8_t v1, uint8_t v2) {

  /* v1 = destination operand, v2 = source operand */
  uint16_t        dst;

  dst = (uint16_t)v1 - (uint16_t)v2;
  flag_szp8(cpu, (uint8_t)dst);
  if (dst & 0xFF00) {
    cpu->cf = 1;
  } else {
    cpu->cf = 0;
  }

  if ((dst ^ v1) & (v1 ^ v2) & 0x80) {
    cpu->of = 1;
  } else {
    cpu->of = 0;
  }

  if ((v1 ^ v2 ^ dst) & 0x10) {
    cpu->af = 1;
  } else {
    cpu->af = 0;
  }
}

void flag_sub16(CPU_t* cpu, uint16_t v1, uint16_t v2) {

  /* v1 = destination operand, v2 = source operand */
  uint32_t        dst;

  dst = (uint32_t)v1 - (uint32_t)v2;
  flag_szp16(cpu, (uint16_t)dst);
  if (dst & 0xFFFF0000) {
    cpu->cf = 1;
  } else {
    cpu->cf = 0;
  }

  if ((dst ^ v1) & (v1 ^ v2) & 0x8000) {
    cpu->of = 1;
  } else {
    cpu->of = 0;
  }

  if ((v1 ^ v2 ^ dst) & 0x10) {
    cpu->af = 1;
  } else {
    cpu->af = 0;
  }
}

void op_adc8(CPU_t* cpu) {
  cpu->res8 = cpu->oper1b + cpu->oper2b + cpu->cf;
  flag_adc8(cpu, cpu->oper1b, cpu->oper2b, cpu->cf);
}

void op_adc16(CPU_t* cpu) {
  cpu->res16 = cpu->oper1 + cpu->oper2 + cpu->cf;
  flag_adc16(cpu, cpu->oper1, cpu->oper2, cpu->cf);
}

void op_add8(CPU_t* cpu) {
  cpu->res8 = cpu->oper1b + cpu->oper2b;
  flag_add8(cpu, cpu->oper1b, cpu->oper2b);
}

void op_add16(CPU_t* cpu) {
  cpu->res16 = cpu->oper1 + cpu->oper2;
  flag_add16(cpu, cpu->oper1, cpu->oper2);
}

void op_and8(CPU_t* cpu) {
  cpu->res8 = cpu->oper1b & cpu->oper2b;
  flag_log8(cpu, cpu->res8);
}

void op_and16(CPU_t* cpu) {
  cpu->res16 = cpu->oper1 & cpu->oper2;
  flag_log16(cpu, cpu->res16);
}

void op_or8(CPU_t* cpu) {
  cpu->res8 = cpu->oper1b | cpu->oper2b;
  flag_log8(cpu, cpu->res8);
}

void op_or16(CPU_t* cpu) {
  cpu->res16 = cpu->oper1 | cpu->oper2;
  flag_log16(cpu, cpu->res16);
}

void op_xor8(CPU_t* cpu) {
  cpu->res8 = cpu->oper1b ^ cpu->oper2b;
  flag_log8(cpu, cpu->res8);
}

void op_xor16(CPU_t* cpu) {
  cpu->res16 = cpu->oper1 ^ cpu->oper2;
  flag_log16(cpu, cpu->res16);
}

void op_sub8(CPU_t* cpu) {
  cpu->res8 = cpu->oper1b - cpu->oper2b;
  flag_sub8(cpu, cpu->oper1b, cpu->oper2b);
}

void op_sub16(CPU_t* cpu) {
  cpu->res16 = cpu->oper1 - cpu->oper2;
  flag_sub16(cpu, cpu->oper1, cpu->oper2);
}

void op_sbb8(CPU_t* cpu) {
  cpu->res8 = cpu->oper1b - (cpu->oper2b + cpu->cf);
  flag_sbb8(cpu, cpu->oper1b, cpu->oper2b, cpu->cf);
}

void op_sbb16(CPU_t* cpu) {
  cpu->res16 = cpu->oper1 - (cpu->oper2 + cpu->cf);
  flag_sbb16(cpu, cpu->oper1, cpu->oper2, cpu->cf);
}

void getea(CPU_t* cpu, uint8_t rmval) {
  uint32_t        tempea;

  tempea = 0;
  switch (cpu->mode) {
    case 0:
      switch (rmval) {
        case 0:
          tempea = cpu->regs.wordregs[regbx] + cpu->regs.wordregs[regsi];
          break;
        case 1:
          tempea = cpu->regs.wordregs[regbx] + cpu->regs.wordregs[regdi];
          break;
        case 2:
          tempea = cpu->regs.wordregs[regbp] + cpu->regs.wordregs[regsi];
          break;
        case 3:
          tempea = cpu->regs.wordregs[regbp] + cpu->regs.wordregs[regdi];
          break;
        case 4:
          tempea = cpu->regs.wordregs[regsi];
          break;
        case 5:
          tempea = cpu->regs.wordregs[regdi];
          break;
        case 6:
          tempea = cpu->disp16;
          break;
        case 7:
          tempea = cpu->regs.wordregs[regbx];
          break;
      }
    break;

    case 1:
    case 2:
      switch (rmval) {
        case 0:
          tempea = cpu->regs.wordregs[regbx] + cpu->regs.wordregs[regsi] + cpu->disp16;
          break;
        case 1:
          tempea = cpu->regs.wordregs[regbx] + cpu->regs.wordregs[regdi] + cpu->disp16;
          break;
        case 2:
          tempea = cpu->regs.wordregs[regbp] + cpu->regs.wordregs[regsi] + cpu->disp16;
          break;
        case 3:
          tempea = cpu->regs.wordregs[regbp] + cpu->regs.wordregs[regdi] + cpu->disp16;
          break;
        case 4:
          tempea = cpu->regs.wordregs[regsi] + cpu->disp16;
          break;
        case 5:
          tempea = cpu->regs.wordregs[regdi] + cpu->disp16;
          break;
        case 6:
          tempea = cpu->regs.wordregs[regbp] + cpu->disp16;
          break;
        case 7:
          tempea = cpu->regs.wordregs[regbx] + cpu->disp16;
          break;
      }
    break;
  }

  cpu->ea = (tempea & 0xFFFF) + (cpu->useseg << 4);
}

void push(CPU_t* cpu, uint16_t pushval) {
  cpu->regs.wordregs[regsp] = cpu->regs.wordregs[regsp] - 2;
  putmem16(cpu, cpu->segregs[regss], cpu->regs.wordregs[regsp], pushval);
}

uint16_t pop(CPU_t* cpu) {

  uint16_t        tempval;

  tempval = getmem16(cpu, cpu->segregs[regss], cpu->regs.wordregs[regsp]);
  cpu->regs.wordregs[regsp] = cpu->regs.wordregs[regsp] + 2;
  return tempval;
}

void cpu_reset(CPU_t* cpu) {
  //uint16_t i;
  //for (i = 0; i < 256; i++) {
  //        cpu->int_callback[i] = NULL;
  //}
  cpu->segregs[regcs] = 0xFFFF;
  cpu->ip = 0x0000;
  cpu->hltstate = 0;
  cpu->trap_toggle = 0;
}

uint16_t readrm16(CPU_t* cpu, uint8_t rmval) {
  //debug_log(DEBUG_INFO, "cpu readmem16\r\n");
  cpu->usedRegister = 0xFF;
  if (cpu->mode < 3) {
    getea(cpu, rmval);
    return cpu_read(cpu, cpu->ea) | ((uint16_t)cpu_read(cpu, cpu->ea + 1) << 8);
  } else {
    cpu->usedRegister = rmval;
    return getreg16(cpu, rmval);
  }
}

uint8_t readrm8(CPU_t* cpu, uint8_t rmval) {
  //debug_log(DEBUG_INFO, "cpu readmem8\r\n");
  if (cpu->mode < 3) {
    getea(cpu, rmval);
    return cpu_read(cpu, cpu->ea);
  } else {
    return getreg8(cpu, rmval);
  }
}

void writerm16(CPU_t* cpu, uint8_t rmval, uint16_t value) {
  //debug_log(DEBUG_INFO, "cpu writemem16\r\n");
  cpu->usedRegister = 0xFF;
  if (cpu->mode < 3) {
    getea(cpu, rmval);
    cpu_write(cpu, cpu->ea, value & 0xFF);
    cpu_write(cpu, cpu->ea + 1, value >> 8);
  } else {
    cpu->usedRegister = rmval;
    putreg16(cpu, rmval, value);
  }
}

void writerm8(CPU_t* cpu, uint8_t rmval, uint8_t value) {
  //debug_log(DEBUG_INFO, "cpu writemem8\r\n");
  if (cpu->mode < 3) {
    getea(cpu, rmval);
    cpu_write(cpu, cpu->ea, value);
  } else {
    putreg8(cpu, rmval, value);
  }
}

uint8_t op_grp2_8(CPU_t* cpu, uint8_t cnt) {

  uint16_t        s;
  uint16_t        shift;
  uint16_t        oldcf;
  uint16_t        msb;

  s = cpu->oper1b;
  oldcf = cpu->cf;

  switch (cpu->reg) {
    case 0: /* ROL r/m8 */
#ifdef DEBUG_DISASM
      debug_log(DEBUG_DETAIL, "rol\n");
#endif

      for (shift = 1; shift <= cnt; shift++) {
        if (s & 0x80) {
        cpu->cf = 1;
        } else {
          cpu->cf = 0;
        }

        s = s << 1;
        s = s | cpu->cf;
      }

      if (cnt == 1) {
        //cpu->of = cpu->cf ^ ( (s >> 7) & 1);
        if ((s & 0x80) && cpu->cf) {
          cpu->of = 1;
        } else {
          cpu->of = 0;
        }
      } else {
        cpu->of = 0;
      }
      break;

    case 1: /* ROR r/m8 */
#ifdef DEBUG_DISASM
      debug_log(DEBUG_DETAIL, "ror\n");
#endif
      for (shift = 1; shift <= cnt; shift++) {
        cpu->cf = s & 1;
        s = (s >> 1) | (cpu->cf << 7);
      }

      if (cnt == 1) {
        cpu->of = (s >> 7) ^ ((s >> 6) & 1);
      }
      break;

    case 2: /* RCL r/m8 */
#ifdef DEBUG_DISASM
      debug_log(DEBUG_DETAIL, "rcl\n");
#endif
      for (shift = 1; shift <= cnt; shift++) {
        oldcf = cpu->cf;
        if (s & 0x80) {
          cpu->cf = 1;
        } else {
          cpu->cf = 0;
        }

        s = s << 1;
        s = s | oldcf;
      }

      if (cnt == 1) {
        cpu->of = cpu->cf ^ ((s >> 7) & 1);
      }
      break;

    case 3: /* RCR r/m8 */
#ifdef DEBUG_DISASM
      debug_log(DEBUG_DETAIL, "rcr\n");
#endif
      for (shift = 1; shift <= cnt; shift++) {
        oldcf = cpu->cf;
        cpu->cf = s & 1;
        s = (s >> 1) | (oldcf << 7);
      }

      if (cnt == 1) {
        cpu->of = (s >> 7) ^ ((s >> 6) & 1);
      }
      break;

    case 4:
    case 6: /* SHL r/m8 */
#ifdef DEBUG_DISASM
      debug_log(DEBUG_DETAIL, "shl\n");
#endif
      for (shift = 1; shift <= cnt; shift++) {
        if (s & 0x80) {
          cpu->cf = 1;
        } else {
          cpu->cf = 0;
        }

        s = (s << 1) & 0xFF;
      }

      if ((cnt == 1) && (cpu->cf == (s >> 7))) {
        cpu->of = 0;
      } else {
        cpu->of = 1;
      }

      flag_szp8(cpu, (uint8_t)s);
      break;

    case 5: /* SHR r/m8 */
#ifdef DEBUG_DISASM
      debug_log(DEBUG_DETAIL, "shr\n");
#endif
      if ((cnt == 1) && (s & 0x80)) {
        cpu->of = 1;
      } else {
        cpu->of = 0;
      }

      for (shift = 1; shift <= cnt; shift++) {
        cpu->cf = s & 1;
        s = s >> 1;
      }

      flag_szp8(cpu, (uint8_t)s);
      break;

    case 7: /* SAR r/m8 */
#ifdef DEBUG_DISASM
      debug_log(DEBUG_DETAIL, "sar\n");
#endif
      for (shift = 1; shift <= cnt; shift++) {
        msb = s & 0x80;
        cpu->cf = s & 1;
        s = (s >> 1) | msb;
      }

      cpu->of = 0;
      flag_szp8(cpu, (uint8_t)s);
      break;
  }

  return s & 0xFF;
}

uint16_t op_grp2_16(CPU_t* cpu, uint8_t cnt) {

  uint32_t        s;
  uint32_t        shift;
  uint32_t        oldcf;
  uint32_t        msb;

  s = cpu->oper1;
  oldcf = cpu->cf;
  //#ifdef CPU_LIMIT_SHIFT_COUNT
  //      cnt &= 0x1F;
  //#endif
  switch (cpu->reg) {
      case 0: /* ROL r/m8 */
#ifdef DEBUG_DISASM
        debug_log(DEBUG_DETAIL, "rol\n");
#endif
        for (shift = 1; shift <= cnt; shift++) {
          if (s & 0x8000) {
            cpu->cf = 1;
          } else {
            cpu->cf = 0;
          }

          s = s << 1;
          s = s | cpu->cf;
        }

        if (cnt == 1) {
          cpu->of = cpu->cf ^ ((s >> 15) & 1);
        }
        break;

      case 1: /* ROR r/m8 */
#ifdef DEBUG_DISASM
        debug_log(DEBUG_DETAIL, "ror\n");
#endif
        for (shift = 1; shift <= cnt; shift++) {
          cpu->cf = s & 1;
          s = (s >> 1) | (cpu->cf << 15);
        }

        if (cnt == 1) {
          cpu->of = (s >> 15) ^ ((s >> 14) & 1);
        }
        break;

      case 2: /* RCL r/m8 */
#ifdef DEBUG_DISASM
        debug_log(DEBUG_DETAIL, "rcl\n");
#endif
        for (shift = 1; shift <= cnt; shift++) {
          oldcf = cpu->cf;
          if (s & 0x8000) {
            cpu->cf = 1;
          } else {
            cpu->cf = 0;
          }

          s = s << 1;
          s = s | oldcf;
        }

        if (cnt == 1) {
          cpu->of = cpu->cf ^ ((s >> 15) & 1);
        }
        break;

      case 3: /* RCR r/m8 */
#ifdef DEBUG_DISASM
        debug_log(DEBUG_DETAIL, "rcr\n");
#endif
        for (shift = 1; shift <= cnt; shift++) {
          oldcf = cpu->cf;
          cpu->cf = s & 1;
          s = (s >> 1) | (oldcf << 15);
        }

        if (cnt == 1) {
          cpu->of = (s >> 15) ^ ((s >> 14) & 1);
        }
        break;

      case 4:
      case 6: /* SHL r/m8 */
#ifdef DEBUG_DISASM
        debug_log(DEBUG_DETAIL, "shl\n");
#endif
        for (shift = 1; shift <= cnt; shift++) {
          if (s & 0x8000) {
            cpu->cf = 1;
          } else {
            cpu->cf = 0;
          }

          s = (s << 1) & 0xFFFF;
        }

        if ((cnt == 1) && (cpu->cf == (s >> 15))) {
          cpu->of = 0;
        } else {
          cpu->of = 1;
        }

        flag_szp16(cpu, (uint16_t)s);
        break;

      case 5: /* SHR r/m8 */
#ifdef DEBUG_DISASM
        debug_log(DEBUG_DETAIL, "shr\n");
#endif
        if ((cnt == 1) && (s & 0x8000)) {
          cpu->of = 1;
        } else {
          cpu->of = 0;
        }

        for (shift = 1; shift <= cnt; shift++) {
          cpu->cf = s & 1;
          s = s >> 1;
        }

        flag_szp16(cpu, (uint16_t)s);
        break;

      case 7: /* SAR r/m8 */
#ifdef DEBUG_DISASM
        debug_log(DEBUG_DETAIL, "sar\n");
#endif
        for (shift = 1; shift <= cnt; shift++) {
          msb = s & 0x8000;
          cpu->cf = s & 1;
          s = (s >> 1) | msb;
        }

        cpu->of = 0;
        flag_szp16(cpu, (uint16_t)s);
        break;
  }

  return (uint16_t)s & 0xFFFF;
}

void op_div8(CPU_t* cpu, uint16_t valdiv, uint8_t divisor) {
  if (divisor == 0) {
    cpu_intcall(cpu, 0);
    return;
  }

  if ((valdiv / (uint16_t)divisor) > 0xFF) {
    cpu_intcall(cpu, 0);
    return;
  }

  cpu->regs.byteregs[regah] = valdiv % (uint16_t)divisor;
  cpu->regs.byteregs[regal] = valdiv / (uint16_t)divisor;
}

void op_idiv8(CPU_t* cpu, uint16_t valdiv, uint8_t divisor) {
  //TODO: Rewrite IDIV code, I wrote this in 2011. It can be made far more efficient.
  uint16_t        s1;
  uint16_t        s2;
  uint16_t        d1;
  uint16_t        d2;
  int     sign;

  if (divisor == 0) {
    cpu_intcall(cpu, 0);
    return;
  }

  s1 = valdiv;
  s2 = divisor;
  sign = (((s1 ^ s2) & 0x8000) != 0);
  s1 = (s1 < 0x8000) ? s1 : ((~s1 + 1) & 0xffff);
  s2 = (s2 < 0x8000) ? s2 : ((~s2 + 1) & 0xffff);
  d1 = s1 / s2;
  d2 = s1 % s2;
  if (d1 & 0xFF00) {
    cpu_intcall(cpu, 0);
    return;
  }

  if (sign) {
    d1 = (~d1 + 1) & 0xff;
    d2 = (~d2 + 1) & 0xff;
  }

  cpu->regs.byteregs[regah] = (uint8_t)d2;
  cpu->regs.byteregs[regal] = (uint8_t)d1;
}

void op_grp3_8(CPU_t* cpu) {
  cpu->oper1 = signext(cpu->oper1b);
  cpu->oper2 = signext(cpu->oper2b);
  switch (cpu->reg) {
    case 0:
    case 1: /* TEST */
#ifdef DEBUG_DISASM
      debug_log(DEBUG_DETAIL, "test\n");
#endif
      flag_log8(cpu, cpu->oper1b & getmem8(cpu, cpu->segregs[regcs], cpu->ip));
      StepIP(cpu, 1);
      break;

    case 2: /* NOT */
#ifdef DEBUG_DISASM
      debug_log(DEBUG_DETAIL, "not\n");
#endif
      cpu->res8 = ~cpu->oper1b;
      break;

    case 3: /* NEG */
#ifdef DEBUG_DISASM
      debug_log(DEBUG_DETAIL, "neg\n");
#endif
      cpu->res8 = (~cpu->oper1b) + 1;
      flag_sub8(cpu, 0, cpu->oper1b);
      if (cpu->res8 == 0) {
        cpu->cf = 0;
      } else {
        cpu->cf = 1;
      }
      break;

    case 4: /* MUL */
#ifdef DEBUG_DISASM
      debug_log(DEBUG_DETAIL, "mul\n");
#endif
      cpu->temp1 = (uint32_t)cpu->oper1b * (uint32_t)cpu->regs.byteregs[regal];
      cpu->regs.wordregs[regax] = cpu->temp1 & 0xFFFF;
      flag_szp8(cpu, (uint8_t)cpu->temp1);
      if (cpu->regs.byteregs[regah]) {
        cpu->cf = 1;
        cpu->of = 1;
      } else {
        cpu->cf = 0;
        cpu->of = 0;
      }
      //#ifdef CPU_CLEAR_ZF_ON_MUL
      cpu->zf = 0;
      //#endif
      break;

    case 5: /* IMUL */
#ifdef DEBUG_DISASM
      debug_log(DEBUG_DETAIL, "imul\n");
#endif
      cpu->oper1 = signext(cpu->oper1b);
      cpu->temp1 = signext(cpu->regs.byteregs[regal]);
      cpu->temp2 = cpu->oper1;
      if ((cpu->temp1 & 0x80) == 0x80) {
        cpu->temp1 = cpu->temp1 | 0xFFFFFF00;
      }

      if ((cpu->temp2 & 0x80) == 0x80) {
        cpu->temp2 = cpu->temp2 | 0xFFFFFF00;
      }

      cpu->temp3 = (cpu->temp1 * cpu->temp2) & 0xFFFF;
      cpu->regs.wordregs[regax] = cpu->temp3 & 0xFFFF;
      if (cpu->regs.byteregs[regah]) {
        cpu->cf = 1;
        cpu->of = 1;
      } else {
        cpu->cf = 0;
        cpu->of = 0;
      }
      //#ifdef CPU_CLEAR_ZF_ON_MUL
      cpu->zf = 0;
      //#endif
      break;

    case 6: /* DIV */
#ifdef DEBUG_DISASM
      debug_log(DEBUG_DETAIL, "div\n");
#endif
      op_div8(cpu, cpu->regs.wordregs[regax], cpu->oper1b);
      break;

    case 7: /* IDIV */
#ifdef DEBUG_DISASM
      debug_log(DEBUG_DETAIL, "idiv\n");
#endif
      op_idiv8(cpu, cpu->regs.wordregs[regax], cpu->oper1b);
      break;
  }
}

void op_div16(CPU_t* cpu, uint32_t valdiv, uint16_t divisor) {
  if (divisor == 0) {
    cpu_intcall(cpu, 0);
    return;
  }

  if ((valdiv / (uint32_t)divisor) > 0xFFFF) {
    cpu_intcall(cpu, 0);
    return;
  }

  cpu->regs.wordregs[regdx] = valdiv % (uint32_t)divisor;
  cpu->regs.wordregs[regax] = valdiv / (uint32_t)divisor;
}

void op_idiv16(CPU_t* cpu, uint32_t valdiv, uint16_t divisor) {
  //TODO: Rewrite IDIV code, I wrote this in 2011. It can be made far more efficient.
  uint32_t        d1;
  uint32_t        d2;
  uint32_t        s1;
  uint32_t        s2;
  int     sign;

  if (divisor == 0) {
    cpu_intcall(cpu, 0);
    return;
  }

  s1 = valdiv;
  s2 = divisor;
  s2 = (s2 & 0x8000) ? (s2 | 0xffff0000) : s2;
  sign = (((s1 ^ s2) & 0x80000000) != 0);
  s1 = (s1 < 0x80000000) ? s1 : ((~s1 + 1) & 0xffffffff);
  s2 = (s2 < 0x80000000) ? s2 : ((~s2 + 1) & 0xffffffff);
  d1 = s1 / s2;
  d2 = s1 % s2;
  if (d1 & 0xFFFF0000) {
    cpu_intcall(cpu, 0);
    return;
  }

  if (sign) {
    d1 = (~d1 + 1) & 0xffff;
    d2 = (~d2 + 1) & 0xffff;
  }

  cpu->regs.wordregs[regax] = d1;
  cpu->regs.wordregs[regdx] = d2;
}

void op_grp3_16(CPU_t* cpu) {
  switch (cpu->reg) {
    case 0:
    case 1: /* TEST */
#ifdef DEBUG_DISASM
      debug_log(DEBUG_DETAIL, "test\n");
#endif
      flag_log16(cpu, cpu->oper1 & getmem16(cpu, cpu->segregs[regcs], cpu->ip));
      StepIP(cpu, 2);
    break;

    case 2: /* NOT */
#ifdef DEBUG_DISASM
      debug_log(DEBUG_DETAIL, "not\n");
#endif
      cpu->res16 = ~cpu->oper1;
      break;

    case 3: /* NEG */
#ifdef DEBUG_DISASM
      debug_log(DEBUG_DETAIL, "neg\n");
#endif
      cpu->res16 = (~cpu->oper1) + 1;
      flag_sub16(cpu, 0, cpu->oper1);
      if (cpu->res16) {
        cpu->cf = 1;
      } else {
        cpu->cf = 0;
      }
      break;

    case 4: /* MUL */
#ifdef DEBUG_DISASM
      debug_log(DEBUG_DETAIL, "mul\n");
#endif
      cpu->temp1 = (uint32_t)cpu->oper1 * (uint32_t)cpu->regs.wordregs[regax];
      cpu->regs.wordregs[regax] = cpu->temp1 & 0xFFFF;
      cpu->regs.wordregs[regdx] = cpu->temp1 >> 16;
      flag_szp16(cpu, (uint16_t)cpu->temp1);
      if (cpu->regs.wordregs[regdx]) {
        cpu->cf = 1;
        cpu->of = 1;
      } else {
        cpu->cf = 0;
        cpu->of = 0;
      }
      //#ifdef CPU_CLEAR_ZF_ON_MUL
      cpu->zf = 0;
      //#endif
      break;

    case 5: /* IMUL */
#ifdef DEBUG_DISASM
      debug_log(DEBUG_DETAIL, "imul\n");
#endif
      cpu->temp1 = cpu->regs.wordregs[regax];
      cpu->temp2 = cpu->oper1;
      if (cpu->temp1 & 0x8000) {
        cpu->temp1 |= 0xFFFF0000;
      }

      if (cpu->temp2 & 0x8000) {
        cpu->temp2 |= 0xFFFF0000;
      }

      cpu->temp3 = cpu->temp1 * cpu->temp2;
      cpu->regs.wordregs[regax] = cpu->temp3 & 0xFFFF;        /* into register ax */
      cpu->regs.wordregs[regdx] = cpu->temp3 >> 16;   /* into register dx */
      if (cpu->regs.wordregs[regdx]) {
        cpu->cf = 1;
        cpu->of = 1;
      } else {
        cpu->cf = 0;
        cpu->of = 0;
      }
      //#ifdef CPU_CLEAR_ZF_ON_MUL
      cpu->zf = 0;
      //#endif
      break;

    case 6: /* DIV */
#ifdef DEBUG_DISASM
      debug_log(DEBUG_DETAIL, "div\n");
#endif
      op_div16(cpu, ((uint32_t)cpu->regs.wordregs[regdx] << 16) + cpu->regs.wordregs[regax], cpu->oper1);
      break;

    case 7: /* IDIV */
#ifdef DEBUG_DISASM
      debug_log(DEBUG_DETAIL, "idiv\n");
#endif
      op_idiv16(cpu, ((uint32_t)cpu->regs.wordregs[regdx] << 16) + cpu->regs.wordregs[regax], cpu->oper1);
      break;
  }
}

void op_grp5(CPU_t* cpu) {
  switch (cpu->reg) {
    case 0: /* INC Ev */
#ifdef DEBUG_DISASM
      debug_log(DEBUG_DETAIL, "inc\n");
#endif
      cpu->oper2 = 1;
      cpu->tempcf = cpu->cf;
      op_add16(cpu);
      cpu->cf = cpu->tempcf;
      writerm16(cpu, cpu->rm, cpu->res16);
      break;

    case 1: /* DEC Ev */
#ifdef DEBUG_DISASM
      debug_log(DEBUG_DETAIL, "dec\n");
#endif
      cpu->oper2 = 1;
      cpu->tempcf = cpu->cf;
      op_sub16(cpu);
      cpu->cf = cpu->tempcf;
      writerm16(cpu, cpu->rm, cpu->res16);
      break;

    case 2: /* CALL Ev */
      push(cpu, cpu->ip);
      cpu->ip = cpu->oper1;
#ifdef DEBUG_DISASM
      debug_log(DEBUG_DETAIL, "call %04X:%04X\n", cpu->segregs[regcs], cpu->ip);
#endif
      break;

    case 3: /* CALL Mp */
      push(cpu, cpu->segregs[regcs]);
      push(cpu, cpu->ip);
      getea(cpu, cpu->rm);
      cpu->ip = (uint16_t)cpu_read(cpu, cpu->ea) + (uint16_t)cpu_read(cpu, cpu->ea + 1) * 256;
      cpu->segregs[regcs] = (uint16_t)cpu_read(cpu, cpu->ea + 2) + (uint16_t)cpu_read(cpu, cpu->ea + 3) * 256;
#ifdef DEBUG_DISASM
      debug_log(DEBUG_DETAIL, "call %04X:%04X\n", cpu->segregs[regcs], cpu->ip);
#endif
      break;

    case 4: /* JMP Ev */
      cpu->ip = cpu->oper1;
#ifdef DEBUG_DISASM
      debug_log(DEBUG_DETAIL, "jmp %04X:%04X\n", cpu->segregs[regcs], cpu->ip);
#endif
      break;

    case 5: /* JMP Mp */
      getea(cpu, cpu->rm);
      cpu->ip = (uint16_t)cpu_read(cpu, cpu->ea) + (uint16_t)cpu_read(cpu, cpu->ea + 1) * 256;
      cpu->segregs[regcs] = (uint16_t)cpu_read(cpu, cpu->ea + 2) + (uint16_t)cpu_read(cpu, cpu->ea + 3) * 256;
#ifdef DEBUG_DISASM
      debug_log(DEBUG_DETAIL, "jmp %04X:%04X\n", cpu->segregs[regcs], cpu->ip);
#endif
      break;

    case 6: /* PUSH Ev */
#ifdef DEBUG_DISASM
      debug_log(DEBUG_DETAIL, "push\n");
#endif
      push(cpu, cpu->oper1);
      break;
  }
}

void cpu_intcall(CPU_t* cpu, uint8_t intnum) {
#ifdef DEBUG_CPU
  //if ( intnum != 0x23) {
  debug_log(DEBUG_DETAIL, "[cpu] interrupt: 0x%02X\r\n", intnum);
  //}
#endif

  //if (cpu->int_callback[intnum] != NULL) {
  //        (*cpu->int_callback[intnum])(cpu, intnum);
  //        return;
  //}

  push(cpu, makeflagsword(cpu));
  push(cpu, cpu->segregs[regcs]);
  push(cpu, cpu->ip);
  cpu->segregs[regcs] = getmem16(cpu, 0, (uint16_t)intnum * 4 + 2);
  cpu->ip = getmem16(cpu, 0, (uint16_t)intnum * 4);
  cpu->ifl = 0;
  cpu->tf = 0;
}

void cpu_interruptCheck(CPU_t* cpu) {
  /* get next interrupt from the i8259, if any */
  //if (!cpu->trap_toggle && (cpu->ifl && (i8259->irr & (~i8259->imr)))) {
  //debug_log(DEBUG_DETAIL, "[CPU] int check: ifl: %d haveInt: %d\n", cpu->ifl, i8259_haveInt());
  if (!cpu->trap_toggle && (cpu->ifl && i8259_haveInt())) {
    cpu->hltstate = 0;
    cpu_intcall(cpu, i8259_nextintr());
  }
}

//void cpu_exec(CPU_t* cpu, uint32_t execloops) {
uint32_t cpu_exec(CPU_t* cpu) {

  uint32_t loopcount = 0;
  
  uint8_t docontinue;         // for string manipulation instructions
  static uint16_t firstip;    // for string manipulation instructions

  loopcount++;

  if (cpu->trap_toggle) {
    cpu_intcall(cpu, 1);
  }

  if (cpu->tf) {
    cpu->trap_toggle = 1;
  } else {
    cpu->trap_toggle = 0;
  }

  if (cpu->hltstate) {
    return loopcount;
  }

  cpu->reptype = 0;
  cpu->segoverride = 0;
  cpu->useseg = cpu->segregs[regds];
  docontinue = 0;
  firstip = cpu->ip;

  while (!docontinue) {
    cpu->segregs[regcs] = cpu->segregs[regcs] & 0xFFFF;
    cpu->ip = cpu->ip & 0xFFFF;
    cpu->savecs = cpu->segregs[regcs];
    cpu->saveip = cpu->ip;
    cpu->opcode = getmem8(cpu, cpu->segregs[regcs], cpu->ip);
#ifdef DEBUG_CPU
    if (cpu->segregs[regcs] != prevSeg) {
      //debug_log(DEBUG_DETAIL, "[cpu] New CS! exec: Addr: %04X:%04X, opcode: %02X\r\n", cpu->segregs[regcs], cpu->ip, cpu->opcode);
      prevSeg = cpu->segregs[regcs];
    }
#endif

#ifdef DEBUG_CPU
    debug_log(DEBUG_DETAIL, "[cpu] exec: Addr: %04X:%04X, opcode: %02X\n", cpu->segregs[regcs], cpu->ip, cpu->opcode);
    debug_log(DEBUG_DETAIL, "[cpu] regs: AX: %04X, BX: %04X, CX: %04X, DX: %04X\n", cpu->regs.wordregs[regax], cpu->regs.wordregs[regbx], cpu->regs.wordregs[regcx], cpu->regs.wordregs[regdx]);
    debug_log(DEBUG_DETAIL, "[cpu] regs: SI: %04X, DI: %04X, BP: %04X, SP: %04X\n", cpu->regs.wordregs[regsi], cpu->regs.wordregs[regdi], cpu->regs.wordregs[regbp], cpu->regs.wordregs[regsp]);
    debug_log(DEBUG_DETAIL, "[cpu] regs: CS: %04X, DS: %04X, ES: %04X, SS: %04X\n", cpu->segregs[regcs], cpu->segregs[regds], cpu->segregs[reges], cpu->segregs[regss]);
    //uint8_t	tempcf, oldcf, cf, pf, af, zf, sf, tf, ifl, df, of, mode, reg, rm;
#endif

    //debug_log(DEBUG_DETAIL, "[cpu] int flag: %d\n", cpu->ifl);

/*
    if (cpu->segregs[regcs] == 0xFC01 && (cpu->ip >=0x86 && cpu->ip <=0xBB)) {
      debug_log(DEBUG_DETAIL, "[cpu] exec: Addr: %04X:%04X, opcode: %02X\r\n", cpu->segregs[regcs], cpu->ip, cpu->opcode);
      debug_log(DEBUG_DETAIL, "[cpu] regs: AX: %04X, BX: %04X, CX: %04X, DX: %04X\r\n", cpu->regs.wordregs[regax], cpu->regs.wordregs[regbx], cpu->regs.wordregs[regcx], cpu->regs.wordregs[regdx]);
      debug_log(DEBUG_DETAIL, "[cpu] regs: SI: %04X, DI: %04X, BP: %04X, SP: %04X\r\n", cpu->regs.wordregs[regsi], cpu->regs.wordregs[regdi], cpu->regs.wordregs[regbp], cpu->regs.wordregs[regsp]);
      debug_log(DEBUG_DETAIL, "[cpu] regs: CS: %04X, DS: %04X, ES: %04X, SS: %04X\r\n", cpu->segregs[regcs], cpu->segregs[regds], cpu->segregs[reges], cpu->segregs[regss]);
      ramDump(cpu->segregs[regds] << 4);
      ramDump(cpu->segregs[reges] << 4, 0x7FF);
      ramDump((cpu->segregs[regss] << 4) + cpu->regs.wordregs[regbp], 0xff);
    }
*/

    StepIP(cpu, 1);

    switch (cpu->opcode) {
      // segment prefix check
      case 0x26:      // segment ES
      case 0x2E:      // segment CS
      case 0x36:      // segment SS
      case 0x3E:  {   // segment DS
        uint8_t segTmp;
        segTmp = (cpu->opcode & B00011000) >> 3;
#ifdef DEBUG_DISASM
        debug_log(DEBUG_DETAIL, "[DASM] [new] %04X:%04X, op: %02X\t\t", cpu->savecs, cpu->saveip, cpu->opcode);
        debug_log(DEBUG_DETAIL, "segment reg: %s\n", cpuRegNamesChar[segTmp + 8]);
#endif
        cpu->useseg = cpu->segregs[segTmp];
        cpu->segoverride = 1;
        loopcount +=2;
        break;
      }

      // repetition prefix check
      case 0xF3:  {   // REP/REPE/REPZ
#ifdef DEBUG_DISASM
        debug_log(DEBUG_DETAIL, "[DASM] [new] %04X:%04X, op: %02X\t\t", cpu->savecs, cpu->saveip, cpu->opcode);
        debug_log(DEBUG_DETAIL, "REP/REPE/REPZ\n");
#endif
        cpu->reptype = 1;
        loopcount +=6;
        break;
      }
      case 0xF2:  {   // REPNE/REPNZ
#ifdef DEBUG_DISASM
        debug_log(DEBUG_DETAIL, "[DASM] [new] %04X:%04X, op: %02X\t\t", cpu->savecs, cpu->saveip, cpu->opcode);
        debug_log(DEBUG_DETAIL, "REPNE/REPNZ\n");
#endif
        cpu->reptype = 2;
        loopcount +=6;
        break;
      }
      default:  {
        docontinue = 1;
        break;
      }
    }
  }

  //cpu->totalexec++;
  switch (cpu->opcode) {
    // Processor control
    case 0x90: {    // 90 NOP
#ifdef DEBUG_DISASM
      debug_log(DEBUG_DETAIL, "[DASM] [new] %04X:%04X, op: %02X\t\tnop\n", cpu->savecs, cpu->saveip, cpu->opcode);
#endif
      loopcount +=3;
      break;
    }
    case 0xF0: {    // F0 LOCK
#ifdef DEBUG_DISASM
      debug_log(DEBUG_DETAIL, "[DASM] [new] %04X:%04X, op: %02X\t\tlock\n", cpu->savecs, cpu->saveip, cpu->opcode);
#endif
      loopcount +=2;
      break;
    }
    case 0xF4: {    // F4 HLT
#ifdef DEBUG_DISASM
      debug_log(DEBUG_DETAIL, "[DASM] [new] %04X:%04X, op: %02X\t\thlt\n", cpu->savecs, cpu->saveip, cpu->opcode);
#endif
      cpu->hltstate = 1;
      loopcount +=2;
      break;
    }
    case 0xF5: {    // F5 CMC
#ifdef DEBUG_DISASM
      debug_log(DEBUG_DETAIL, "[DASM] [new] %04X:%04X, op: %02X\t\tcmc\n", cpu->savecs, cpu->saveip, cpu->opcode);
#endif
      if (!cpu->cf) {
        cpu->cf = 1;
      } else {
        cpu->cf = 0;
      }
      loopcount +=2;
      break;
    }
    case 0xF8: {    // F8 CLC
#ifdef DEBUG_DISASM
      debug_log(DEBUG_DETAIL, "[DASM] [new] %04X:%04X, op: %02X\t\tclc\n", cpu->savecs, cpu->saveip, cpu->opcode);
#endif
      cpu->cf = 0;
      loopcount +=2;
      break;
    }
    case 0xF9: {    // F9 STC
#ifdef DEBUG_DISASM
      debug_log(DEBUG_DETAIL, "[DASM] [new] %04X:%04X, op: %02X\t\tstc\n", cpu->savecs, cpu->saveip, cpu->opcode);
#endif
      cpu->cf = 1;
      loopcount +=2;
      break;
    }
    case 0xFA: {    // FA CLI
#ifdef DEBUG_DISASM
      debug_log(DEBUG_DETAIL, "[DASM] [new] %04X:%04X, op: %02X\t\tcli\n", cpu->savecs, cpu->saveip, cpu->opcode);
#endif
      cpu->ifl = 0;
      loopcount +=2;
      break;
    }
    case 0xFB: {    // FB STI
#ifdef DEBUG_DISASM
      debug_log(DEBUG_DETAIL, "[DASM] [new] %04X:%04X, op: %02X\t\tsti\n", cpu->savecs, cpu->saveip, cpu->opcode);
#endif
      cpu->ifl = 1;
      loopcount +=2;
      break;
    }
    case 0xFC: {    // FC CLD
#ifdef DEBUG_DISASM
      debug_log(DEBUG_DETAIL, "[DASM] [new] %04X:%04X, op: %02X\t\tcld\n", cpu->savecs, cpu->saveip, cpu->opcode);
#endif
      cpu->df = 0;
      loopcount +=2;
      break;
    }
    case 0xFD: {    // FD STD
#ifdef DEBUG_DISASM
      debug_log(DEBUG_DETAIL, "[DASM] [new] %04X:%04X, op: %02X\t\tstd\n", cpu->savecs, cpu->saveip, cpu->opcode);
#endif
      cpu->df = 1;
      loopcount +=2;
      break;
    }
    case 0xD8 ... 0xDF: {    // escape to x87 FPU (unsupported)
#ifdef DEBUG_DISASM
      debug_log(DEBUG_INFO, "[8087] PRE exec: Addr: %04X:%04X, opcode: %02X\n", cpu->segregs[regcs], cpu->ip, cpu->opcode);
#endif
      do8087(cpu);
      loopcount +=7;
      break;
    }
    case 0x9B: {    // 9B WAIT
#ifdef DEBUG_DISASM
      debug_log(DEBUG_DETAIL, "[DASM] [new] %04X:%04X, op: %02X\t\twait\n", cpu->savecs, cpu->saveip, cpu->opcode);
#endif
      loopcount +=3;
      break;
    }
    
    // String manipulation
    case 0xA4: {    // A4 MOVSB
#ifdef DEBUG_DISASM
      debug_log(DEBUG_DETAIL, "[DASM] [not implemented] %04X:%04X, opcode: %02X\tmovsb\r\n", cpu->segregs[regcs], cpu->ip, cpu->opcode);
#endif
      if (cpu->reptype && (cpu->regs.wordregs[regcx] == 0)) {
        break;
      }

      putmem8(cpu, cpu->segregs[reges], cpu->regs.wordregs[regdi], getmem8(cpu, cpu->useseg, cpu->regs.wordregs[regsi]));
      if (cpu->df) {
        cpu->regs.wordregs[regsi] = cpu->regs.wordregs[regsi] - 1;
        cpu->regs.wordregs[regdi] = cpu->regs.wordregs[regdi] - 1;
      } else {
        cpu->regs.wordregs[regsi] = cpu->regs.wordregs[regsi] + 1;
        cpu->regs.wordregs[regdi] = cpu->regs.wordregs[regdi] + 1;
      }

      if (cpu->reptype) {
        cpu->regs.wordregs[regcx] = cpu->regs.wordregs[regcx] - 1;
      }

      loopcount +=17;
      loopcount++;
      if (!cpu->reptype) {
        break;
      }

      cpu->ip = firstip;
      break;
    }
    case 0xA5: {    // A5 MOVSW
#ifdef DEBUG_DISASM
      debug_log(DEBUG_DETAIL, "[DASM] [not implemented] %04X:%04X, opcode: %02X\tmovsw\r\n", cpu->segregs[regcs], cpu->ip, cpu->opcode);
#endif
      if (cpu->reptype && (cpu->regs.wordregs[regcx] == 0)) {
        break;
      }

      putmem16(cpu, cpu->segregs[reges], cpu->regs.wordregs[regdi], getmem16(cpu, cpu->useseg, cpu->regs.wordregs[regsi]));
      if (cpu->df) {
        cpu->regs.wordregs[regsi] = cpu->regs.wordregs[regsi] - 2;
        cpu->regs.wordregs[regdi] = cpu->regs.wordregs[regdi] - 2;
      } else {
        cpu->regs.wordregs[regsi] = cpu->regs.wordregs[regsi] + 2;
        cpu->regs.wordregs[regdi] = cpu->regs.wordregs[regdi] + 2;
      }

      if (cpu->reptype) {
        cpu->regs.wordregs[regcx] = cpu->regs.wordregs[regcx] - 1;
      }
      loopcount +=17;
      loopcount++;
      if (!cpu->reptype) {
        break;
      }

      cpu->ip = firstip;
      break;
    }
    case 0xA6: {    // A6 CMPSB
#ifdef DEBUG_DISASM
      debug_log(DEBUG_DETAIL, "[DASM] [not implemented] %04X:%04X, opcode: %02X\tcmpsb\r\n", cpu->segregs[regcs], cpu->ip, cpu->opcode);
#endif
      if (cpu->reptype && (cpu->regs.wordregs[regcx] == 0)) {
        break;
      }

      cpu->oper1b = getmem8(cpu, cpu->useseg, cpu->regs.wordregs[regsi]);
      cpu->oper2b = getmem8(cpu, cpu->segregs[reges], cpu->regs.wordregs[regdi]);
      if (cpu->df) {
        cpu->regs.wordregs[regsi] = cpu->regs.wordregs[regsi] - 1;
        cpu->regs.wordregs[regdi] = cpu->regs.wordregs[regdi] - 1;
      } else {
        cpu->regs.wordregs[regsi] = cpu->regs.wordregs[regsi] + 1;
        cpu->regs.wordregs[regdi] = cpu->regs.wordregs[regdi] + 1;
      }

      flag_sub8(cpu, cpu->oper1b, cpu->oper2b);
      if (cpu->reptype) {
        cpu->regs.wordregs[regcx] = cpu->regs.wordregs[regcx] - 1;
      }

      if ((cpu->reptype == 1) && !cpu->zf) {
        break;
      } else if ((cpu->reptype == 2) && (cpu->zf == 1)) {
        break;
      }
      loopcount +=22;
      loopcount++;
      if (!cpu->reptype) {
        break;
      }

      cpu->ip = firstip;
      break;
    }
    case 0xA7: {    // A7 CMPSW
#ifdef DEBUG_DISASM
      debug_log(DEBUG_DETAIL, "[DASM] [not implemented] %04X:%04X, opcode: %02X\tcmpsw\r\n", cpu->segregs[regcs], cpu->ip, cpu->opcode);
#endif
      if (cpu->reptype && (cpu->regs.wordregs[regcx] == 0)) {
        break;
      }

      cpu->oper1 = getmem16(cpu, cpu->useseg, cpu->regs.wordregs[regsi]);
      cpu->oper2 = getmem16(cpu, cpu->segregs[reges], cpu->regs.wordregs[regdi]);
      if (cpu->df) {
        cpu->regs.wordregs[regsi] = cpu->regs.wordregs[regsi] - 2;
        cpu->regs.wordregs[regdi] = cpu->regs.wordregs[regdi] - 2;
      } else {
        cpu->regs.wordregs[regsi] = cpu->regs.wordregs[regsi] + 2;
        cpu->regs.wordregs[regdi] = cpu->regs.wordregs[regdi] + 2;
      }

      flag_sub16(cpu, cpu->oper1, cpu->oper2);
      if (cpu->reptype) {
        cpu->regs.wordregs[regcx] = cpu->regs.wordregs[regcx] - 1;
      }

      if ((cpu->reptype == 1) && !cpu->zf) {
        break;
      }

      if ((cpu->reptype == 2) && (cpu->zf == 1)) {
        break;
      }
      loopcount +=22;
      loopcount++;
      if (!cpu->reptype) {
        break;
      }

      cpu->ip = firstip;
      break;
    }
    case 0xAA: {    // AA STOSB
#ifdef DEBUG_DISASM
      debug_log(DEBUG_DETAIL, "[DASM] [new] %04X:%04X, op: %02X\t\t", cpu->savecs, cpu->saveip, cpu->opcode);
      debug_log(DEBUG_DETAIL, "stosb\tcx: %04Xh\n", cpu->regs.wordregs[regcx]);
#endif
      if (cpu->reptype && (cpu->regs.wordregs[regcx] == 0)) {
        break;
      }

      putmem8(cpu, cpu->segregs[reges], cpu->regs.wordregs[regdi], cpu->regs.byteregs[regal]);
      if (cpu->df) {
        cpu->regs.wordregs[regdi] = cpu->regs.wordregs[regdi] - 1;
      } else {
        cpu->regs.wordregs[regdi] = cpu->regs.wordregs[regdi] + 1;
      }

      if (cpu->reptype) {
        cpu->regs.wordregs[regcx] = cpu->regs.wordregs[regcx] - 1;
      }
      loopcount +=10;
      loopcount++;
      if (!cpu->reptype) {
        break;
      }

      cpu->ip = firstip;
      break;
    }
    case 0xAB: {    // AB STOSW
#ifdef DEBUG_DISASM
      debug_log(DEBUG_DETAIL, "[DASM] [not implemented] %04X:%04X, opcode: %02X\tstosw\r\n", cpu->segregs[regcs], cpu->ip, cpu->opcode);
#endif
      if (cpu->reptype && (cpu->regs.wordregs[regcx] == 0)) {
        break;
      }

      putmem16(cpu, cpu->segregs[reges], cpu->regs.wordregs[regdi], cpu->regs.wordregs[regax]);
      if (cpu->df) {
        cpu->regs.wordregs[regdi] = cpu->regs.wordregs[regdi] - 2;
      } else {
        cpu->regs.wordregs[regdi] = cpu->regs.wordregs[regdi] + 2;
      }

      if (cpu->reptype) {
        cpu->regs.wordregs[regcx] = cpu->regs.wordregs[regcx] - 1;
      }
      loopcount +=10;
      loopcount++;
      if (!cpu->reptype) {
        break;
      }

      cpu->ip = firstip;
      break;
    }
    case 0xAC: {    // AC LODSB
#ifdef DEBUG_DISASM
      debug_log(DEBUG_DETAIL, "[DASM] [not implemented] %04X:%04X, opcode: %02X\tlodsb\r\n", cpu->segregs[regcs], cpu->ip, cpu->opcode);
#endif
      if (cpu->reptype && (cpu->regs.wordregs[regcx] == 0)) {
        break;
      }

      cpu->regs.byteregs[regal] = getmem8(cpu, cpu->useseg, cpu->regs.wordregs[regsi]);
      if (cpu->df) {
        cpu->regs.wordregs[regsi] = cpu->regs.wordregs[regsi] - 1;
      } else {
        cpu->regs.wordregs[regsi] = cpu->regs.wordregs[regsi] + 1;
      }

      if (cpu->reptype) {
        cpu->regs.wordregs[regcx] = cpu->regs.wordregs[regcx] - 1;
      }
      loopcount +=12;
      loopcount++;
      if (!cpu->reptype) {
        break;
      }

      cpu->ip = firstip;
      break;
    }
    case 0xAD: {    // AD LODSW
#ifdef DEBUG_DISASM
      debug_log(DEBUG_DETAIL, "[DASM] [not implemented] %04X:%04X, opcode: %02X\tlodsw\r\n", cpu->segregs[regcs], cpu->ip, cpu->opcode);
#endif
      if (cpu->reptype && (cpu->regs.wordregs[regcx] == 0)) {
        break;
      }

      cpu->oper1 = getmem16(cpu, cpu->useseg, cpu->regs.wordregs[regsi]);
      cpu->regs.wordregs[regax] = cpu->oper1;
      if (cpu->df) {
        cpu->regs.wordregs[regsi] = cpu->regs.wordregs[regsi] - 2;
      } else {
        cpu->regs.wordregs[regsi] = cpu->regs.wordregs[regsi] + 2;
      }

      if (cpu->reptype) {
        cpu->regs.wordregs[regcx] = cpu->regs.wordregs[regcx] - 1;
      }
      loopcount +=12;
      loopcount++;
      if (!cpu->reptype) {
        break;
      }

      cpu->ip = firstip;
      break;
    }
    case 0xAE: {    // AE SCASB
#ifdef DEBUG_DISASM
      debug_log(DEBUG_DETAIL, "[DASM] [not implemented] %04X:%04X, opcode: %02X\tscasb\r\n", cpu->segregs[regcs], cpu->ip, cpu->opcode);
#endif
      if (cpu->reptype && (cpu->regs.wordregs[regcx] == 0)) {
        break;
      }

      cpu->oper1b = cpu->regs.byteregs[regal];
      cpu->oper2b = getmem8(cpu, cpu->segregs[reges], cpu->regs.wordregs[regdi]);
      flag_sub8(cpu, cpu->oper1b, cpu->oper2b);
      if (cpu->df) {
        cpu->regs.wordregs[regdi] = cpu->regs.wordregs[regdi] - 1;
      } else {
        cpu->regs.wordregs[regdi] = cpu->regs.wordregs[regdi] + 1;
      }

      if (cpu->reptype) {
        cpu->regs.wordregs[regcx] = cpu->regs.wordregs[regcx] - 1;
      }

      if ((cpu->reptype == 1) && !cpu->zf) {
        break;
      } else if ((cpu->reptype == 2) && (cpu->zf == 1)) {
        break;
      }
      loopcount +=15;
      loopcount++;
      if (!cpu->reptype) {
        break;
      }

      cpu->ip = firstip;
      break;
    }
    case 0xAF: {    // AF SCASW
#ifdef DEBUG_DISASM
      debug_log(DEBUG_DETAIL, "[DASM] [not implemented] %04X:%04X, opcode: %02X\tscasw\r\n", cpu->segregs[regcs], cpu->ip, cpu->opcode);
#endif
      if (cpu->reptype && (cpu->regs.wordregs[regcx] == 0)) {
        break;
      }

      cpu->oper1 = cpu->regs.wordregs[regax];
      cpu->oper2 = getmem16(cpu, cpu->segregs[reges], cpu->regs.wordregs[regdi]);
      flag_sub16(cpu, cpu->oper1, cpu->oper2);
      if (cpu->df) {
        cpu->regs.wordregs[regdi] = cpu->regs.wordregs[regdi] - 2;
      } else {
        cpu->regs.wordregs[regdi] = cpu->regs.wordregs[regdi] + 2;
      }

      if (cpu->reptype) {
        cpu->regs.wordregs[regcx] = cpu->regs.wordregs[regcx] - 1;
      }

      if ((cpu->reptype == 1) && !cpu->zf) {
        break;
      } else if ((cpu->reptype == 2) && (cpu->zf == 1)) { //did i fix a typo bug? this used to be & instead of &&
        break;
      }
      loopcount +=15;
      loopcount++;
      if (!cpu->reptype) {
        break;
      }

      cpu->ip = firstip;
      break;
    }

    // Control transfer
    case 0x9A: {    // 9A CALL Ap
      cpu->oper1 = getmem16(cpu, cpu->segregs[regcs], cpu->ip);
      StepIP(cpu, 2);
      cpu->oper2 = getmem16(cpu, cpu->segregs[regcs], cpu->ip);
      StepIP(cpu, 2);
      push(cpu, cpu->segregs[regcs]);
      push(cpu, cpu->ip);
      cpu->ip = cpu->oper1;
      cpu->segregs[regcs] = cpu->oper2;
      loopcount +=28;
#ifdef DEBUG_DISASM
      debug_log(DEBUG_DETAIL, "[DASM] [new] %04X:%04X, opcode: %02X\t\t", cpu->savecs, cpu->saveip, cpu->opcode);
      debug_log(DEBUG_DETAIL, "[not implemented] call: %04X:%04X\r\n", cpu->segregs[regcs], cpu->ip);
#endif
      break;
    }
    case 0xE8: {    // E8 CALL Jv
      cpu->oper1 = getmem16(cpu, cpu->segregs[regcs], cpu->ip);
      StepIP(cpu, 2);
      push(cpu, cpu->ip);
      cpu->ip = cpu->ip + cpu->oper1;
      loopcount +=11;
#ifdef DEBUG_DISASM
      debug_log(DEBUG_DETAIL, "[DASM] [new] %04X:%04X, opcode: %02X\t\t", cpu->savecs, cpu->saveip, cpu->opcode);
      debug_log(DEBUG_DETAIL, "[not impelemented] call near %04X:%04X\n", cpu->segregs[regcs], cpu->ip);
#endif
      break;
    }
    case 0xE9: {    // E9 JMP Jv
      cpu->oper1 = getmem16(cpu, cpu->segregs[regcs], cpu->ip);
      StepIP(cpu, 2);
      cpu->ip = cpu->ip + cpu->oper1;
      loopcount +=7;
#ifdef DEBUG_DISASM
      debug_log(DEBUG_DETAIL, "[DASM] [new] %04X:%04X, opcode: %02X\t\t", cpu->savecs, cpu->saveip, cpu->opcode);
      debug_log(DEBUG_DETAIL, "jmp %04X:%04X\n", cpu->segregs[regcs], cpu->ip);
#endif
      break;
    }
    case 0xEA: {    // EA JMP Ap
      cpu->oper1 = getmem16(cpu, cpu->segregs[regcs], cpu->ip);
      StepIP(cpu, 2);
      cpu->oper2 = getmem16(cpu, cpu->segregs[regcs], cpu->ip);
      cpu->ip = cpu->oper1;
      cpu->segregs[regcs] = cpu->oper2;
      loopcount +=7;
#ifdef DEBUG_DISASM
      debug_log(DEBUG_DETAIL, "[DASM] [new] %04X:%04X, op: %02X\t\t", cpu->savecs, cpu->saveip, cpu->opcode);
      debug_log(DEBUG_DETAIL, "jmp far %04X:%04X\n", cpu->segregs[regcs], cpu->ip);
#endif
      break;
    }
    case 0xEB: {    // EB JMP Jb
      cpu->oper1 = signext(getmem8(cpu, cpu->segregs[regcs], cpu->ip));
      StepIP(cpu, 1);
      cpu->ip = cpu->ip + cpu->oper1;
      loopcount +=7;
#ifdef DEBUG_DISASM
      debug_log(DEBUG_DETAIL, "[DASM] [new] %04X:%04X, op: %02X\t\t", cpu->savecs, cpu->saveip, cpu->opcode);
      debug_log(DEBUG_DETAIL, "jmp short %04X:%04X\n", cpu->segregs[regcs], cpu->ip);
#endif
      break;
    }
    case 0xC2: {    // C2 RET Iw
      cpu->oper1 = getmem16(cpu, cpu->segregs[regcs], cpu->ip);
      cpu->ip = pop(cpu);
      cpu->regs.wordregs[regsp] = cpu->regs.wordregs[regsp] + cpu->oper1;
      loopcount +=12;
#ifdef DEBUG_DISASM
      debug_log(DEBUG_DETAIL, "[DASM] [new] %04X:%04X, op: %02X\t\t", cpu->savecs, cpu->saveip, cpu->opcode);
      debug_log(DEBUG_DETAIL, "ret %02X\n", cpu->oper1);
#endif
      break;
    }
    case 0xC3: {    // C3 RET
      cpu->ip = pop(cpu);
      loopcount +=8;
#ifdef DEBUG_DISASM
      debug_log(DEBUG_DETAIL, "[DASM] [new] %04X:%04X, op: %02X\t\t", cpu->savecs, cpu->saveip, cpu->opcode);
      debug_log(DEBUG_DETAIL, "ret\n");
#endif
      break;
    }
    case 0xCA: {    // CA RETF Iw
      cpu->oper1 = getmem16(cpu, cpu->segregs[regcs], cpu->ip);
      cpu->ip = pop(cpu);
      cpu->segregs[regcs] = pop(cpu);
      cpu->regs.wordregs[regsp] = cpu->regs.wordregs[regsp] + cpu->oper1;
      loopcount +=17;
#ifdef DEBUG_DISASM
      debug_log(DEBUG_DETAIL, "[DASM] [new] %04X:%04X, op: %02X\t\t", cpu->savecs, cpu->saveip, cpu->opcode);
      debug_log(DEBUG_DETAIL, "retf %02X\n", cpu->oper1);
#endif
      break;
    }
    case 0xCB: {    // CB RETF
      cpu->ip = pop(cpu);
      cpu->segregs[regcs] = pop(cpu);
      loopcount +=18;
#ifdef DEBUG_DISASM
      debug_log(DEBUG_DETAIL, "[DASM] [new] %04X:%04X, op: %02X\t\t", cpu->savecs, cpu->saveip, cpu->opcode);
      debug_log(DEBUG_DETAIL, "retf\n");
#endif
      break;
    }

    case 0x70: {    // 70 JO Jb
#ifdef DEBUG_DISASM
      debug_log(DEBUG_DETAIL, "[DASM] %04X:%04X, opcode: %02X\t", cpu->segregs[regcs], cpu->ip, cpu->opcode);
#endif
      cpu->temp16 = signext(getmem8(cpu, cpu->segregs[regcs], cpu->ip));
      StepIP(cpu, 1);
      loopcount +=4;
      if (cpu->of) {
        cpu->ip = cpu->ip + cpu->temp16;
        loopcount +=4;
      }
#ifdef DEBUG_DISASM
      debug_log(DEBUG_DETAIL, "jo %04X:%04X\r\n", cpu->segregs[regcs], cpu->ip);
#endif
      break;
    }
    case 0x71: {    // 71 JNO Jb
#ifdef DEBUG_DISASM
      debug_log(DEBUG_DETAIL, "[DASM] %04X:%04X, opcode: %02X\t", cpu->segregs[regcs], cpu->ip, cpu->opcode);
#endif
      cpu->temp16 = signext(getmem8(cpu, cpu->segregs[regcs], cpu->ip));
      StepIP(cpu, 1);
      loopcount +=4;
      if (!cpu->of) {
        cpu->ip = cpu->ip + cpu->temp16;
        loopcount +=4;
      }
#ifdef DEBUG_DISASM
      debug_log(DEBUG_DETAIL, "jno %04X:%04X\r\n", cpu->segregs[regcs], cpu->ip);
#endif
      break;
    }
    case 0x72: {    // 72 JB Jb (JNAE)
#ifdef DEBUG_DISASM
      debug_log(DEBUG_DETAIL, "[DASM] %04X:%04X, opcode: %02X\t", cpu->segregs[regcs], cpu->ip, cpu->opcode);
#endif
      cpu->temp16 = signext(getmem8(cpu, cpu->segregs[regcs], cpu->ip));
      StepIP(cpu, 1);
      loopcount +=4;
      if (cpu->cf) {
        cpu->ip = cpu->ip + cpu->temp16;
        loopcount +=4;
      }
#ifdef DEBUG_DISASM
      debug_log(DEBUG_DETAIL, "jb %04X:%04X\r\n", cpu->segregs[regcs], cpu->ip);
#endif
      break;
    }
    case 0x73: {    // 73 JNB Jb (JAE)
#ifdef DEBUG_DISASM
      debug_log(DEBUG_DETAIL, "[DASM] %04X:%04X, opcode: %02X\t", cpu->segregs[regcs], cpu->ip, cpu->opcode);
#endif
      cpu->temp16 = signext(getmem8(cpu, cpu->segregs[regcs], cpu->ip));
      StepIP(cpu, 1);
      loopcount +=4;
      if (!cpu->cf) {
        cpu->ip = cpu->ip + cpu->temp16;
        loopcount +=4;
      }
#ifdef DEBUG_DISASM
      debug_log(DEBUG_DETAIL, "jnb %04X:%04X\r\n", cpu->segregs[regcs], cpu->ip);
#endif
      break;
    }
    case 0x74: {    // 74 JZ Jb (JE)
#ifdef DEBUG_DISASM
      debug_log(DEBUG_DETAIL, "[DASM] %04X:%04X, opcode: %02X\t", cpu->segregs[regcs], cpu->ip, cpu->opcode);
#endif
      cpu->temp16 = signext(getmem8(cpu, cpu->segregs[regcs], cpu->ip));
      StepIP(cpu, 1);
      loopcount +=4;
      if (cpu->zf) {
        cpu->ip = cpu->ip + cpu->temp16;
        loopcount +=4;
      }
#ifdef DEBUG_DISASM
      debug_log(DEBUG_DETAIL, "jz %04X:%04X\r\n", cpu->segregs[regcs], cpu->ip);
#endif
      break;
    }
    case 0x75: {    // 75 JNZ Jb (JNE)
      cpu->temp16 = signext(getmem8(cpu, cpu->segregs[regcs], cpu->ip));
      StepIP(cpu, 1);
      loopcount +=4;

      if (!cpu->zf) {
        cpu->ip = cpu->ip + cpu->temp16;
        loopcount +=4;
      }
#ifdef DEBUG_DISASM
      debug_log(DEBUG_DETAIL, "jnz %04X:%04X\tflag Z: %01X\n", cpu->segregs[regcs], cpu->ip, cpu->zf);
#endif
      break;
    }
    case 0x76: {    // 76 JBE Jb (JNA)
      cpu->temp16 = signext(getmem8(cpu, cpu->segregs[regcs], cpu->ip));
      StepIP(cpu, 1);
      loopcount +=4;
      if (cpu->cf || cpu->zf) {
        cpu->ip = cpu->ip + cpu->temp16;
        loopcount +=4;
      }
#ifdef DEBUG_DISASM
      debug_log(DEBUG_DETAIL, "jbe %04X:%04X\tflag Z: %01X, C: %01X\n", cpu->segregs[regcs], cpu->ip, cpu->zf, cpu->cf);
#endif
      break;
    }
    case 0x77: {    // 77 JA Jb (JNBE)
      cpu->temp16 = signext(getmem8(cpu, cpu->segregs[regcs], cpu->ip));
      StepIP(cpu, 1);
      loopcount +=4;
      if (!cpu->cf && !cpu->zf) {
        cpu->ip = cpu->ip + cpu->temp16;
        loopcount +=4;
      }
#ifdef DEBUG_DISASM
      debug_log(DEBUG_DETAIL, "ja %04X:%04X\tflag Z: %01X, C: %01X\n", cpu->segregs[regcs], cpu->ip, cpu->zf, cpu->cf);
#endif
      break;
    }
    case 0x78: {    // 78 JS Jb
      cpu->temp16 = signext(getmem8(cpu, cpu->segregs[regcs], cpu->ip));
      StepIP(cpu, 1);
      loopcount +=4;
      if (cpu->sf) {
        cpu->ip = cpu->ip + cpu->temp16;
        loopcount +=4;
      }
#ifdef DEBUG_DISASM
      debug_log(DEBUG_DETAIL, "js %04X:%04X\tflag S: %01X\n", cpu->segregs[regcs], cpu->ip, cpu->sf);
#endif
      break;
    }
    case 0x79: {    // 79 JNS Jb
      cpu->temp16 = signext(getmem8(cpu, cpu->segregs[regcs], cpu->ip));
      StepIP(cpu, 1);
      loopcount +=4;
      if (!cpu->sf) {
        cpu->ip = cpu->ip + cpu->temp16;
        loopcount +=4;
      }
#ifdef DEBUG_DISASM
      debug_log(DEBUG_DETAIL, "jns %04X:%04X\tflag S: %01X\n", cpu->segregs[regcs], cpu->ip, cpu->sf);
#endif
      break;
    }
    case 0x7A: {    // 7A JPE Jb (JP)
      cpu->temp16 = signext(getmem8(cpu, cpu->segregs[regcs], cpu->ip));
      StepIP(cpu, 1);
      loopcount +=4;
      if (cpu->pf) {
        cpu->ip = cpu->ip + cpu->temp16;
        loopcount +=4;
      }
#ifdef DEBUG_DISASM
      debug_log(DEBUG_DETAIL, "jpe %04X:%04X\tflag P: %01X\n", cpu->segregs[regcs], cpu->ip, cpu->pf);
#endif
      break;
    }
    case 0x7B: {    // 7B JPO Jb (JNP)
      cpu->temp16 = signext(getmem8(cpu, cpu->segregs[regcs], cpu->ip));
      StepIP(cpu, 1);
      loopcount +=4;
      if (!cpu->pf) {
        cpu->ip = cpu->ip + cpu->temp16;
        loopcount +=4;
      }
#ifdef DEBUG_DISASM
      debug_log(DEBUG_DETAIL, "jpo %04X:%04X\tflag P: %01X\n", cpu->segregs[regcs], cpu->ip, cpu->pf);
#endif
      break;
    }
    case 0x7C: {    // 7C JL Jb (JNGE)
      cpu->temp16 = signext(getmem8(cpu, cpu->segregs[regcs], cpu->ip));
      StepIP(cpu, 1);
      loopcount +=4;
      if (cpu->sf != cpu->of) {
        cpu->ip = cpu->ip + cpu->temp16;
        loopcount +=4;
      }
#ifdef DEBUG_DISASM
      debug_log(DEBUG_DETAIL, "jl %04X:%04X\tflag S: %01X, O: %01X\n", cpu->segregs[regcs], cpu->ip, cpu->sf, cpu->of);
#endif
      break;
    }
    case 0x7D: {    // 7D JGE Jb (JNL)
      cpu->temp16 = signext(getmem8(cpu, cpu->segregs[regcs], cpu->ip));
      StepIP(cpu, 1);
      loopcount +=4;
      if (cpu->sf == cpu->of) {
        cpu->ip = cpu->ip + cpu->temp16;
        loopcount +=4;
      }
#ifdef DEBUG_DISASM
      debug_log(DEBUG_DETAIL, "jge %04X:%04X\tflag S: %01X, O: %01X\n", cpu->segregs[regcs], cpu->ip, cpu->sf, cpu->of);
#endif
      break;
    }
    case 0x7E: {    // 7E JLE Jb (JNG)
      cpu->temp16 = signext(getmem8(cpu, cpu->segregs[regcs], cpu->ip));
      StepIP(cpu, 1);
      loopcount +=4;
      if ((cpu->sf != cpu->of) || cpu->zf) {
        cpu->ip = cpu->ip + cpu->temp16;
        loopcount +=4;
      }
#ifdef DEBUG_DISASM
      debug_log(DEBUG_DETAIL, "jle %04X:%04X\tflag S: %01X, O: %01X, Z: %01X\n", cpu->segregs[regcs], cpu->ip, cpu->sf, cpu->of, cpu->zf);
#endif
      break;
    }
    case 0x7F: {    // 7F JG Jb (JNLE)
      cpu->temp16 = signext(getmem8(cpu, cpu->segregs[regcs], cpu->ip));
      StepIP(cpu, 1);
      loopcount +=4;
      if (!cpu->zf && (cpu->sf == cpu->of)) {
        cpu->ip = cpu->ip + cpu->temp16;
        loopcount +=4;
      }
#ifdef DEBUG_DISASM
      debug_log(DEBUG_DETAIL, "jg %04X:%04X\tflag S: %01X, O: %01X, Z: %01X\n", cpu->segregs[regcs], cpu->ip, cpu->sf, cpu->of, cpu->zf);
#endif
      break;
    }

    case 0xE0: {    // E0 LOOPNZ Jb (LOOPNE)
#ifdef DEBUG_DISASM
      debug_log(DEBUG_DETAIL, "[DASM] %04X:%04X opcode: %02X\tloopnz ", cpu->segregs[regcs], cpu->ip, cpu->opcode);
#endif
      cpu->temp16 = signext(getmem8(cpu, cpu->segregs[regcs], cpu->ip));
      StepIP(cpu, 1);
      cpu->regs.wordregs[regcx] = cpu->regs.wordregs[regcx] - 1;
      loopcount +=5;
      if ((cpu->regs.wordregs[regcx]) && !cpu->zf) {
        cpu->ip = cpu->ip + cpu->temp16;
        loopcount +=6;
      }
#ifdef DEBUG_DISASM
      debug_log(DEBUG_DETAIL, "%04X:%04X\r\n", cpu->segregs[regcs], cpu->ip);
#endif
      break;
    }
    case 0xE1: {    // E1 LOOPZ Jb (LLOPE)
#ifdef DEBUG_DISASM
      debug_log(DEBUG_DETAIL, "[DASM] %04X:%04X opcode: %02X\tloopz ", cpu->segregs[regcs], cpu->ip, cpu->opcode);
#endif
      cpu->temp16 = signext(getmem8(cpu, cpu->segregs[regcs], cpu->ip));
      StepIP(cpu, 1);
      cpu->regs.wordregs[regcx] = cpu->regs.wordregs[regcx] - 1;
      loopcount +=5;
      if (cpu->regs.wordregs[regcx] && (cpu->zf == 1)) {
        cpu->ip = cpu->ip + cpu->temp16;
        loopcount +=6;
      }
#ifdef DEBUG_DISASM
      debug_log(DEBUG_DETAIL, "%04X:%04X\r\n", cpu->segregs[regcs], cpu->ip);
#endif
      break;
    }
    case 0xE2: {    // E2 LOOP Jb
      cpu->temp16 = signext(getmem8(cpu, cpu->segregs[regcs], cpu->ip));
      StepIP(cpu, 1);
      cpu->regs.wordregs[regcx] = cpu->regs.wordregs[regcx] - 1;
      loopcount +=5;
      if (cpu->regs.wordregs[regcx]) {
        cpu->ip = cpu->ip + cpu->temp16;
        loopcount +=4;
      }
#ifdef DEBUG_DISASM
      debug_log(DEBUG_DETAIL, "loop %04X:%04X\t cx: %04X\n", cpu->segregs[regcs], cpu->ip, cpu->regs.wordregs[regcx]);
#endif
      break;
    }
    case 0xE3: {    // E3 JCXZ Jb
#ifdef DEBUG_DISASM
      debug_log(DEBUG_DETAIL, "[DASM] %04X:%04X opcode: %02X\tjcxz ", cpu->segregs[regcs], cpu->ip, cpu->opcode);
#endif
      cpu->temp16 = signext(getmem8(cpu, cpu->segregs[regcs], cpu->ip));
      StepIP(cpu, 1);
      loopcount +=5;
      if (!cpu->regs.wordregs[regcx]) {
        cpu->ip = cpu->ip + cpu->temp16;
        loopcount +=4;
      }
#ifdef DEBUG_DISASM
      debug_log(DEBUG_DETAIL, "%04X:%04X\r\n", cpu->segregs[regcs], cpu->ip);
#endif
      break;
    }
    case 0xCC: {    // CC INT 3
#ifdef DEBUG_DISASM
      debug_log(DEBUG_DETAIL, "[DASM] %04X:%04X, opcode: %02X\tint 3\r\n", cpu->segregs[regcs], cpu->ip, cpu->opcode);
#endif
      cpu_intcall(cpu, 3);
      loopcount +=51;
      break;
    }
    case 0xCD: {    // CD INT Ib
      cpu->oper1b = getmem8(cpu, cpu->segregs[regcs], cpu->ip);
#ifdef DEBUG_DISASM
      debug_log(DEBUG_DETAIL, "[DASM] %04X:%04X, opcode: %02X\tint %02X\r\n", cpu->segregs[regcs], cpu->ip, cpu->opcode, cpu->oper1b);
#endif
      StepIP(cpu, 1);
      cpu_intcall(cpu, cpu->oper1b);
      loopcount +=50;
      break;
    }
    case 0xCE: {    // CE INTO
#ifdef DEBUG_DISASM
      debug_log(DEBUG_DETAIL, "[DASM] %04X:%04X, opcode: %02X\tint 0 (overflow)\r\n", cpu->segregs[regcs], cpu->ip, cpu->opcode);
#endif
      if (cpu->of) {
        cpu_intcall(cpu, 4);
        loopcount +=52;
      }
      loopcount +=4;
      break;
    }
    case 0xCF: {    // CF IRET
#ifdef DEBUG_DISASM
      debug_log(DEBUG_DETAIL, "[DASM] %04X:%04X, opcode: %02X\tiret\r\n", cpu->segregs[regcs], cpu->ip, cpu->opcode);
#endif
      cpu->ip = pop(cpu);
      cpu->segregs[regcs] = pop(cpu);
      decodeflagsword(cpu, pop(cpu));

      /*
      * if (net.enabled) net.canrecv = 1;
      */
      //insideInterrupt = false;
      loopcount +=24;
      break;
    }

    // Logic
    case 0xD0: {    // D0 GRP2 Eb 1
#ifdef DEBUG_DISASM
      debug_log(DEBUG_DETAIL, "[DASM] [not implemented] %04X:%04X, opcode: %02X\t", cpu->segregs[regcs], cpu->ip, cpu->opcode);
#endif
      modregrm(cpu);
      cpu->oper1b = readrm8(cpu, cpu->rm);
      writerm8(cpu, cpu->rm, op_grp2_8(cpu, 1));
      loopcount +=15;
      break;
    }
    case 0xD1: {    // D1 GRP2 Ev 1
#ifdef DEBUG_DISASM
      debug_log(DEBUG_DETAIL, "[DASM] [not implemented] %04X:%04X, opcode: %02X\t", cpu->segregs[regcs], cpu->ip, cpu->opcode);
#endif
      modregrm(cpu);
      cpu->oper1 = readrm16(cpu, cpu->rm);
      writerm16(cpu, cpu->rm, op_grp2_16(cpu, 1));
      loopcount +=15;
      break;
    }
    case 0xD2: {    // D2 GRP2 Eb cpu->regs.byteregs[regcl]
#ifdef DEBUG_DISASM
      debug_log(DEBUG_DETAIL, "[DASM] [not implemented] %04X:%04X, opcode: %02X\t", cpu->segregs[regcs], cpu->ip, cpu->opcode);
#endif
      modregrm(cpu);
      cpu->oper1b = readrm8(cpu, cpu->rm);
      writerm8(cpu, cpu->rm, op_grp2_8(cpu, cpu->regs.byteregs[regcl]));
      loopcount +=2;
      break;
    }
    case 0xD3: {    // D3 GRP2 Ev cpu->regs.byteregs[regcl]
#ifdef DEBUG_DISASM
      debug_log(DEBUG_DETAIL, "[DASM] [not implemented] %04X:%04X, opcode: %02X\t", cpu->segregs[regcs], cpu->ip, cpu->opcode);
#endif
      modregrm(cpu);
      cpu->oper1 = readrm16(cpu, cpu->rm);
      writerm16(cpu, cpu->rm, op_grp2_16(cpu, cpu->regs.byteregs[regcl]));
      loopcount +=2;
      break;
    }
    case 0x20: {    // 20 AND Eb Gb
      modregrm(cpu);
      cpu->oper1b = readrm8(cpu, cpu->rm);
      cpu->oper2b = getreg8(cpu, cpu->reg);
      op_and8(cpu);
      writerm8(cpu, cpu->rm, cpu->res8);
      loopcount +=3;
#ifdef DEBUG_DISASM
      debug_log(DEBUG_DETAIL, "[DASM] [not implemented] %04X:%04X, opcode: %02X\tand\r\n", cpu->segregs[regcs], cpu->ip, cpu->opcode);
#endif
      break;
    }
    case 0x21: {    // 21 AND Ev Gv
      modregrm(cpu);
      cpu->oper1 = readrm16(cpu, cpu->rm);
      cpu->oper2 = getreg16(cpu, cpu->reg);
      op_and16(cpu);
      writerm16(cpu, cpu->rm, cpu->res16);
      loopcount +=3;
#ifdef DEBUG_DISASM
      debug_log(DEBUG_DETAIL, "[DASM] [not implemented] %04X:%04X, opcode: %02X\tand\r\n", cpu->segregs[regcs], cpu->ip, cpu->opcode);
#endif
      break;
    }
    case 0x22: {    // 22 AND Gb Eb
      modregrm(cpu);
      cpu->oper1b = getreg8(cpu, cpu->reg);
      cpu->oper2b = readrm8(cpu, cpu->rm);
      op_and8(cpu);
      putreg8(cpu, cpu->reg, cpu->res8);
      loopcount +=3;
#ifdef DEBUG_DISASM
      debug_log(DEBUG_DETAIL, "[DASM] [not implemented] %04X:%04X, opcode: %02X\tand\r\n", cpu->segregs[regcs], cpu->ip, cpu->opcode);
#endif
      break;
    }
    case 0x23: {    // 23 AND Gv Ev
      modregrm(cpu);
      cpu->oper1 = getreg16(cpu, cpu->reg);
      cpu->oper2 = readrm16(cpu, cpu->rm);
      op_and16(cpu);
      putreg16(cpu, cpu->reg, cpu->res16);
      loopcount +=3;
#ifdef DEBUG_DISASM
      debug_log(DEBUG_DETAIL, "[DASM] [not implemented] %04X:%04X, opcode: %02X\tand\r\n", cpu->segregs[regcs], cpu->ip, cpu->opcode);
#endif
      break;
    }
    case 0x24: {    // 24 AND cpu->regs.byteregs[regal] Ib
      cpu->oper1b = cpu->regs.byteregs[regal];
      cpu->oper2b = getmem8(cpu, cpu->segregs[regcs], cpu->ip);
      StepIP(cpu, 1);
      op_and8(cpu);
      cpu->regs.byteregs[regal] = cpu->res8;
      loopcount +=3;
#ifdef DEBUG_DISASM
      debug_log(DEBUG_DETAIL, "[DASM] [not implemented] %04X:%04X, opcode: %02X\tand\r\n", cpu->segregs[regcs], cpu->ip, cpu->opcode);
#endif
      break;
    }
    case 0x25: {    // 25 AND eAX Iv
      cpu->oper1 = cpu->regs.wordregs[regax];
      cpu->oper2 = getmem16(cpu, cpu->segregs[regcs], cpu->ip);
      StepIP(cpu, 2);
      op_and16(cpu);
      cpu->regs.wordregs[regax] = cpu->res16;
      loopcount +=3;
#ifdef DEBUG_DISASM
      debug_log(DEBUG_DETAIL, "[DASM] [not implemented] %04X:%04X, opcode: %02X\tand\r\n", cpu->segregs[regcs], cpu->ip, cpu->opcode);
#endif
      break;
    }
    case 0x84: {    // 84 TEST Gb Eb
      modregrm(cpu);
      cpu->oper1b = getreg8(cpu, cpu->reg);
      cpu->oper2b = readrm8(cpu, cpu->rm);
      flag_log8(cpu, cpu->oper1b & cpu->oper2b);
      loopcount +=3;
#ifdef DEBUG_DISASM
      debug_log(DEBUG_DETAIL, "[DASM] [not implemented] %04X:%04X, opcode: %02X\ttest\r\n", cpu->segregs[regcs], cpu->ip, cpu->opcode);
#endif
      break;
    }
    case 0x85: {    // 85 TEST Gv Ev
      modregrm(cpu);
      cpu->oper1 = getreg16(cpu, cpu->reg);
      cpu->oper2 = readrm16(cpu, cpu->rm);
      flag_log16(cpu, cpu->oper1 & cpu->oper2);
      loopcount +=3;
#ifdef DEBUG_DISASM
      debug_log(DEBUG_DETAIL, "[DASM] [not implemented] %04X:%04X, opcode: %02X\ttest\r\n", cpu->segregs[regcs], cpu->ip, cpu->opcode);
#endif
      break;
    }
    case 0xA8: {    // A8 TEST cpu->regs.byteregs[regal] Ib
#ifdef DEBUG_DISASM
      debug_log(DEBUG_DETAIL, "[DASM] [not implemented] %04X:%04X, opcode: %02X\ttest\r\n", cpu->segregs[regcs], cpu->ip, cpu->opcode);
#endif
      cpu->oper1b = cpu->regs.byteregs[regal];
      cpu->oper2b = getmem8(cpu, cpu->segregs[regcs], cpu->ip);
      StepIP(cpu, 1);
      flag_log8(cpu, cpu->oper1b & cpu->oper2b);
      loopcount +=4;
      break;
    }
    case 0xA9: {    // A9 TEST eAX Iv
#ifdef DEBUG_DISASM
      debug_log(DEBUG_DETAIL, "[DASM] [not implemented] %04X:%04X, opcode: %02X\ttest\r\n", cpu->segregs[regcs], cpu->ip, cpu->opcode);
#endif
      cpu->oper1 = cpu->regs.wordregs[regax];
      cpu->oper2 = getmem16(cpu, cpu->segregs[regcs], cpu->ip);
      StepIP(cpu, 2);
      flag_log16(cpu, cpu->oper1 & cpu->oper2);
      loopcount +=4;
      break;
    }
    case 0x8:  {    // 08 OR Eb Gb
      modregrm(cpu);
      cpu->oper1b = readrm8(cpu, cpu->rm);
      cpu->oper2b = getreg8(cpu, cpu->reg);
      op_or8(cpu);
      writerm8(cpu, cpu->rm, cpu->res8);
      loopcount +=3;
#ifdef DEBUG_DISASM
      debug_log(DEBUG_DETAIL, "[DASM] [not implemented] %04X:%04X, opcode: %02X\tor\r\n", cpu->segregs[regcs], cpu->ip, cpu->opcode);
#endif
      break;
    }
    case 0x9:  {    // 09 OR Ev Gv
      modregrm(cpu);
      cpu->oper1 = readrm16(cpu, cpu->rm);
      cpu->oper2 = getreg16(cpu, cpu->reg);
      op_or16(cpu);
      writerm16(cpu, cpu->rm, cpu->res16);
      loopcount +=3;
#ifdef DEBUG_DISASM
      debug_log(DEBUG_DETAIL, "[DASM] [not implemented] %04X:%04X, opcode: %02X\tor\r\n", cpu->segregs[regcs], cpu->ip, cpu->opcode);
#endif
      break;
    }
    case 0xA:  {    // 0A OR Gb Eb
      modregrm(cpu);
      cpu->oper1b = getreg8(cpu, cpu->reg);
      cpu->oper2b = readrm8(cpu, cpu->rm);
      op_or8(cpu);
      putreg8(cpu, cpu->reg, cpu->res8);
      loopcount +=3;
#ifdef DEBUG_DISASM
      debug_log(DEBUG_DETAIL, "[DASM] [not implemented] %04X:%04X, opcode: %02X\tor\r\n", cpu->segregs[regcs], cpu->ip, cpu->opcode);
#endif
      break;
    }
    case 0xB:  {    // 0B OR Gv Ev
      modregrm(cpu);
      cpu->oper1 = getreg16(cpu, cpu->reg);
      cpu->oper2 = readrm16(cpu, cpu->rm);
      op_or16(cpu);
      //if ((cpu->oper1 == 0xF802) && (cpu->oper2 == 0xF802)) {
      //      cpu->sf = 0;    /* cheap hack to make Wolf 3D think we're a 286 so it plays */
      //}
      putreg16(cpu, cpu->reg, cpu->res16);
      loopcount +=3;
#ifdef DEBUG_DISASM
      debug_log(DEBUG_DETAIL, "[DASM] [not implemented] %04X:%04X, opcode: %02X\tor\r\n", cpu->segregs[regcs], cpu->ip, cpu->opcode);
#endif
      break;
    }
    case 0xC:  {    // 0C OR cpu->regs.byteregs[regal] Ib
      cpu->oper1b = cpu->regs.byteregs[regal];
      cpu->oper2b = getmem8(cpu, cpu->segregs[regcs], cpu->ip);
      StepIP(cpu, 1);
      op_or8(cpu);
      cpu->regs.byteregs[regal] = cpu->res8;
      loopcount +=4;
#ifdef DEBUG_DISASM
      debug_log(DEBUG_DETAIL, "[DASM] [not implemented] %04X:%04X, opcode: %02X\tor\r\n", cpu->segregs[regcs], cpu->ip, cpu->opcode);
#endif
      break;
    }
    case 0xD:  {    // 0D OR eAX Iv
      cpu->oper1 = cpu->regs.wordregs[regax];
      cpu->oper2 = getmem16(cpu, cpu->segregs[regcs], cpu->ip);
      StepIP(cpu, 2);
      op_or16(cpu);
      cpu->regs.wordregs[regax] = cpu->res16;
      loopcount +=4;
#ifdef DEBUG_DISASM
      debug_log(DEBUG_DETAIL, "[DASM] [not implemented] %04X:%04X, opcode: %02X\tor\r\n", cpu->segregs[regcs], cpu->ip, cpu->opcode);
#endif
      break;
    }
    case 0x30: {    // 30 XOR Eb Gb
      modregrm(cpu);
      cpu->oper1b = readrm8(cpu, cpu->rm);
      cpu->oper2b = getreg8(cpu, cpu->reg);
      op_xor8(cpu);
      writerm8(cpu, cpu->rm, cpu->res8);
      loopcount +=3;
#ifdef DEBUG_DISASM
      debug_log(DEBUG_DETAIL, "[DASM] [not implemented] %04X:%04X, opcode: %02X\txor\r\n", cpu->segregs[regcs], cpu->ip, cpu->opcode);
#endif
      break;
    }
    case 0x31: {    // 31 XOR Ev Gv
      modregrm(cpu);
      cpu->oper1 = readrm16(cpu, cpu->rm);
      cpu->oper2 = getreg16(cpu, cpu->reg);
      op_xor16(cpu);
      writerm16(cpu, cpu->rm, cpu->res16);
      loopcount +=3;
#ifdef DEBUG_DISASM
      debug_log(DEBUG_DETAIL, "[DASM] [not implemented] %04X:%04X, opcode: %02X\txor\r\n", cpu->segregs[regcs], cpu->ip, cpu->opcode);
#endif
      break;
    }
    case 0x32: {    // 32 XOR Gb Eb
      modregrm(cpu);
      cpu->oper1b = getreg8(cpu, cpu->reg);
      cpu->oper2b = readrm8(cpu, cpu->rm);
      op_xor8(cpu);
      putreg8(cpu, cpu->reg, cpu->res8);
      loopcount +=3;
#ifdef DEBUG_DISASM
      debug_log(DEBUG_DETAIL, "[DASM] [not implemented] %04X:%04X, opcode: %02X\txor\r\n", cpu->segregs[regcs], cpu->ip, cpu->opcode);
#endif
      break;
    }
    case 0x33: {    // 33 XOR Gv Ev
      modregrm(cpu);
      cpu->oper1 = getreg16(cpu, cpu->reg);
      cpu->oper2 = readrm16(cpu, cpu->rm);
      op_xor16(cpu);
      putreg16(cpu, cpu->reg, cpu->res16);
      loopcount +=3;
#ifdef DEBUG_DISASM
      debug_log(DEBUG_DETAIL, "[DASM] [new] [testing] %04X:%04X, op: %02X\t\t", cpu->savecs, cpu->saveip, cpu->opcode);
      debug_log(DEBUG_DETAIL, "xor %s, %s\n", cpuRegNamesChar[cpu->reg], cpuRegNamesChar[cpu->rm]);
#endif
      break;
    }
    case 0x34: {    // 34 XOR cpu->regs.byteregs[regal] Ib
      cpu->oper1b = cpu->regs.byteregs[regal];
      cpu->oper2b = getmem8(cpu, cpu->segregs[regcs], cpu->ip);
      StepIP(cpu, 1);
      op_xor8(cpu);
      cpu->regs.byteregs[regal] = cpu->res8;
      loopcount +=3;
#ifdef DEBUG_DISASM
      debug_log(DEBUG_DETAIL, "[DASM] [not implemented] %04X:%04X, opcode: %02X\txor\r\n", cpu->segregs[regcs], cpu->ip, cpu->opcode);
#endif
      break;
    }
    case 0x35: {    // 35 XOR eAX Iv
      cpu->oper1 = cpu->regs.wordregs[regax];
      cpu->oper2 = getmem16(cpu, cpu->segregs[regcs], cpu->ip);
      StepIP(cpu, 2);
      op_xor16(cpu);
      cpu->regs.wordregs[regax] = cpu->res16;
      loopcount +=3;
#ifdef DEBUG_DISASM
      debug_log(DEBUG_DETAIL, "[DASM] [not implemented] %04X:%04X, opcode: %02X\txor\r\n", cpu->segregs[regcs], cpu->ip, cpu->opcode);
#endif
      break;
    }

    // Arithmetic
    case 0x0:  {    // 00 ADD Eb Gb
      modregrm(cpu);
      cpu->oper1b = readrm8(cpu, cpu->rm);
      cpu->oper2b = getreg8(cpu, cpu->reg);
      op_add8(cpu);
      writerm8(cpu, cpu->rm, cpu->res8);
      loopcount +=3;
#ifdef DEBUG_DISASM
      debug_log(DEBUG_DETAIL, "[DASM] [not implemented] %04X:%04X, opcode: %02X\tadd\r\n", cpu->segregs[regcs], cpu->ip, cpu->opcode);
#endif
      break;
    }
    case 0x1:  {    // 01 ADD Ev Gv
      modregrm(cpu);
      cpu->oper1 = readrm16(cpu, cpu->rm);
      cpu->oper2 = getreg16(cpu, cpu->reg);
      op_add16(cpu);
      writerm16(cpu, cpu->rm, cpu->res16);
      loopcount +=3;
#ifdef DEBUG_DISASM
      debug_log(DEBUG_DETAIL, "[DASM] [not implemented] %04X:%04X, opcode: %02X\tadd\r\n", cpu->segregs[regcs], cpu->ip, cpu->opcode);
#endif
      break;
    }
    case 0x2:  {    // 02 ADD Gb Eb
      modregrm(cpu);
      cpu->oper1b = getreg8(cpu, cpu->reg);
      cpu->oper2b = readrm8(cpu, cpu->rm);
      op_add8(cpu);
      putreg8(cpu, cpu->reg, cpu->res8);
      loopcount +=3;
#ifdef DEBUG_DISASM
      debug_log(DEBUG_DETAIL, "[DASM] [not implemented] %04X:%04X, opcode: %02X\tadd\r\n", cpu->segregs[regcs], cpu->ip, cpu->opcode);
#endif
      break;
    }
    case 0x3:  {    // 03 ADD Gv Ev
      modregrm(cpu);
      cpu->oper1 = getreg16(cpu, cpu->reg);
      cpu->oper2 = readrm16(cpu, cpu->rm);
      op_add16(cpu);
      putreg16(cpu, cpu->reg, cpu->res16);
      loopcount +=3;
#ifdef DEBUG_DISASM
      debug_log(DEBUG_DETAIL, "[DASM] [not implemented] %04X:%04X, opcode: %02X\tadd\r\n", cpu->segregs[regcs], cpu->ip, cpu->opcode);
#endif
      break;
    }
    case 0x4:  {    // 04 ADD cpu->regs.byteregs[regal] Ib
      cpu->oper1b = cpu->regs.byteregs[regal];
      cpu->oper2b = getmem8(cpu, cpu->segregs[regcs], cpu->ip);
      StepIP(cpu, 1);
      op_add8(cpu);
      cpu->regs.byteregs[regal] = cpu->res8;
      loopcount +=4;
#ifdef DEBUG_DISASM
      debug_log(DEBUG_DETAIL, "[DASM] [not implemented] %04X:%04X, opcode: %02X\tadd\r\n", cpu->segregs[regcs], cpu->ip, cpu->opcode);
#endif
      break;
    }
    case 0x5:  {    // 05 ADD eAX Iv
      cpu->oper1 = cpu->regs.wordregs[regax];
      cpu->oper2 = getmem16(cpu, cpu->segregs[regcs], cpu->ip);
      StepIP(cpu, 2);
      op_add16(cpu);
      cpu->regs.wordregs[regax] = cpu->res16;
      loopcount +=4;
#ifdef DEBUG_DISASM
      debug_log(DEBUG_DETAIL, "[DASM] [not implemented] %04X:%04X, opcode: %02X\tadd\r\n", cpu->segregs[regcs], cpu->ip, cpu->opcode);
#endif
      break;
    }
    case 0x80:
    case 0x82: {    // 80/82 GRP1 Eb Ib
#ifdef DEBUG_DISASM
      debug_log(DEBUG_DETAIL, "[DASM] [not implemented] %04X:%04X, opcode: %02X\t", cpu->segregs[regcs], cpu->ip, cpu->opcode);
#endif
      modregrm(cpu);
      cpu->oper1b = readrm8(cpu, cpu->rm);
      cpu->oper2b = getmem8(cpu, cpu->segregs[regcs], cpu->ip);
      StepIP(cpu, 1);
      switch (cpu->reg) {
        case 0:
#ifdef DEBUG_DISASM
          debug_log(DEBUG_DETAIL, "[not implemented] add\r\n");
#endif
          op_add8(cpu);
          loopcount +=3;
          break;
        case 1:
#ifdef DEBUG_DISASM
          debug_log(DEBUG_DETAIL, "[not implemented] or\r\n");
#endif
          op_or8(cpu);
          loopcount +=3;
          break;
        case 2:
#ifdef DEBUG_DISASM
          debug_log(DEBUG_DETAIL, "[not implemented] adc\r\n");
#endif
          op_adc8(cpu);
          loopcount +=3;
          break;
        case 3:
#ifdef DEBUG_DISASM
          debug_log(DEBUG_DETAIL, "[not implemented] sbb\r\n");
#endif
          op_sbb8(cpu);
          loopcount +=3;
          break;
        case 4:
#ifdef DEBUG_DISASM
          debug_log(DEBUG_DETAIL, "[not implemented] and\r\n");
#endif
          op_and8(cpu);
          loopcount +=3;
          break;
        case 5:
#ifdef DEBUG_DISASM
          debug_log(DEBUG_DETAIL, "[not implemented] sub\r\n");
#endif
          op_sub8(cpu);
          loopcount +=3;
          break;
        case 6:
#ifdef DEBUG_DISASM
          debug_log(DEBUG_DETAIL, "[not implemented] xor\r\n");
#endif
          op_xor8(cpu);
          loopcount +=3;
          break;
        case 7:
#ifdef DEBUG_DISASM
          debug_log(DEBUG_DETAIL, "[not implemented] cmp\r\n");
#endif
          flag_sub8(cpu, cpu->oper1b, cpu->oper2b);
          loopcount +=3;
          break;
        default:
          break;  // to avoid compiler warnings
      }

      if (cpu->reg < 7) {
        writerm8(cpu, cpu->rm, cpu->res8);
        loopcount +=17;
      }
      break;
    }
    case 0x81:      // 81 GRP1 Ev Iv
    case 0x83: {    // 83 GRP1 Ev Ib
      modregrm(cpu);
      cpu->oper1 = readrm16(cpu, cpu->rm);
      if (cpu->opcode == 0x81) {
        cpu->oper2 = getmem16(cpu, cpu->segregs[regcs], cpu->ip);
        StepIP(cpu, 2);
      } else {
        cpu->oper2 = signext(getmem8(cpu, cpu->segregs[regcs], cpu->ip));
        StepIP(cpu, 1);
      }

#ifdef DEBUG_DISASM
      debug_log(DEBUG_DETAIL, "[DASM] [new] %04X:%04X, op: %02X\t\t", cpu->savecs, cpu->saveip, cpu->opcode);
#endif

      switch (cpu->reg) {
        case 0: {
#ifdef DEBUG_DISASM
          debug_log(DEBUG_DETAIL, "[not implemented] add ???, %04Xh\n", cpu->oper2);
#endif
          op_add16(cpu);
          loopcount +=3;
          break;
        }
        case 1: {
#ifdef DEBUG_DISASM
          debug_log(DEBUG_DETAIL, "[not implemented] or\r\n");
#endif
          op_or16(cpu);
          loopcount +=3;
          break;
        }
        case 2: {
#ifdef DEBUG_DISASM
          debug_log(DEBUG_DETAIL, "[not implemented] adc\r\n");
#endif
          op_adc16(cpu);
          loopcount +=3;
          break;
        }
        case 3: {
#ifdef DEBUG_DISASM
          debug_log(DEBUG_DETAIL, "[not implemented] sbb\r\n");
#endif
          op_sbb16(cpu);
          loopcount +=3;
          break;
        }
        case 4: {
#ifdef DEBUG_DISASM
          debug_log(DEBUG_DETAIL, "[not implemented] and\r\n");
#endif
          op_and16(cpu);
          loopcount +=3;
          break;
        }
        case 5: {
#ifdef DEBUG_DISASM
          debug_log(DEBUG_DETAIL, "[not implemented] sbb\r\n");
#endif
          op_sub16(cpu);
          loopcount +=3;
          break;
        }
        case 6: {
#ifdef DEBUG_DISASM
          debug_log(DEBUG_DETAIL, "[not implemented] xor\r\n");
#endif
          op_xor16(cpu);
          loopcount +=3;
          break;
        }
        case 7: {
          flag_sub16(cpu, cpu->oper1, cpu->oper2);
#ifdef DEBUG_DISASM
          debug_log(DEBUG_DETAIL, "[not implemented] cmp %04Xh, %04Xh\n", cpu->oper1, cpu->oper2);
#endif

          loopcount +=3;
          break;
        }
        default:
          break;  // to avoid compiler warnings
      }

      if (cpu->reg < 7) {
        writerm16(cpu, cpu->rm, cpu->res16);
        loopcount +=17;
      }
      break;
    }
    case 0x10: {    // 10 ADC Eb Gb
      modregrm(cpu);
      cpu->oper1b = readrm8(cpu, cpu->rm);
      cpu->oper2b = getreg8(cpu, cpu->reg);
      op_adc8(cpu);
      writerm8(cpu, cpu->rm, cpu->res8);
      loopcount +=3;
#ifdef DEBUG_DISASM
      debug_log(DEBUG_DETAIL, "[DASM] [not implemented] %04X:%04X, opcode: %02X\tadc\r\n", cpu->segregs[regcs], cpu->ip, cpu->opcode);
#endif
      break;
    }
    case 0x11: {    // 11 ADC Ev Gv
      modregrm(cpu);
      cpu->oper1 = readrm16(cpu, cpu->rm);
      cpu->oper2 = getreg16(cpu, cpu->reg);
      op_adc16(cpu);
      writerm16(cpu, cpu->rm, cpu->res16);
      loopcount +=3;
#ifdef DEBUG_DISASM
      debug_log(DEBUG_DETAIL, "[DASM] [not implemented] %04X:%04X, opcode: %02X\tadc\r\n", cpu->segregs[regcs], cpu->ip, cpu->opcode);
#endif
      break;
    }
    case 0x12: {    // 12 ADC Gb Eb
      modregrm(cpu);
      cpu->oper1b = getreg8(cpu, cpu->reg);
      cpu->oper2b = readrm8(cpu, cpu->rm);
      op_adc8(cpu);
      putreg8(cpu, cpu->reg, cpu->res8);
      loopcount +=3;
#ifdef DEBUG_DISASM
      debug_log(DEBUG_DETAIL, "[DASM] [not implemented] %04X:%04X, opcode: %02X\tadc\r\n", cpu->segregs[regcs], cpu->ip, cpu->opcode);
#endif
      break;
    }
    case 0x13: {    // 13 ADC Gv Ev
      modregrm(cpu);
      cpu->oper1 = getreg16(cpu, cpu->reg);
      cpu->oper2 = readrm16(cpu, cpu->rm);
      op_adc16(cpu);
      putreg16(cpu, cpu->reg, cpu->res16);
      loopcount +=3;
#ifdef DEBUG_DISASM
      debug_log(DEBUG_DETAIL, "[DASM] [not implemented] %04X:%04X, opcode: %02X\tadc\r\n", cpu->segregs[regcs], cpu->ip, cpu->opcode);
#endif
      break;
    }
    case 0x14: {    // 14 ADC cpu->regs.byteregs[regal] Ib
      cpu->oper1b = cpu->regs.byteregs[regal];
      cpu->oper2b = getmem8(cpu, cpu->segregs[regcs], cpu->ip);
      StepIP(cpu, 1);
      op_adc8(cpu);
      cpu->regs.byteregs[regal] = cpu->res8;
      loopcount +=3;
#ifdef DEBUG_DISASM
      debug_log(DEBUG_DETAIL, "[DASM] [not implemented] %04X:%04X, opcode: %02X\tadc\r\n", cpu->segregs[regcs], cpu->ip, cpu->opcode);
#endif
      break;
    }
    case 0x15: {    // 15 ADC eAX Iv
      cpu->oper1 = cpu->regs.wordregs[regax];
      cpu->oper2 = getmem16(cpu, cpu->segregs[regcs], cpu->ip);
      StepIP(cpu, 2);
      op_adc16(cpu);
      cpu->regs.wordregs[regax] = cpu->res16;
      loopcount +=3;
#ifdef DEBUG_DISASM
      debug_log(DEBUG_DETAIL, "[DASM] [not implemented] %04X:%04X, opcode: %02X\tadc\r\n", cpu->segregs[regcs], cpu->ip, cpu->opcode);
#endif
      break;
    }
    case 0x40 ... 0x47: {    // INC 16 bit registers ax cx dx bx sp bp si di
      uint8_t tmpReg;
      tmpReg = cpu->opcode & 0x7;
      cpu->oldcf = cpu->cf;
      cpu->oper1 = cpu->regs.wordregs[tmpReg];
      cpu->oper2 = 1;
      op_add16(cpu);
      cpu->cf = cpu->oldcf;
      cpu->regs.wordregs[tmpReg] = cpu->res16;
      loopcount +=2;
#ifdef DEBUG_DISASM
      debug_log(DEBUG_DETAIL, "[DASM] %04X:%04X, opcode: %02X\t\t", cpu->segregs[regcs], cpu->ip, cpu->opcode);
      debug_log(DEBUG_DETAIL, "inc %s\n", cpuRegNamesChar[tmpReg]);
#endif
      break;
    }
    case 0x48 ... 0x4F: {    // DEC 16 bit registers ax cx dx bx sp bp si di
      uint8_t tmpReg;
      tmpReg = cpu->opcode & 0x7;
      cpu->oldcf = cpu->cf;
      cpu->oper1 = cpu->regs.wordregs[tmpReg];
      cpu->oper2 = 1;
      op_sub16(cpu);
      cpu->cf = cpu->oldcf;
      cpu->regs.wordregs[tmpReg] = cpu->res16;
      loopcount +=2;
#ifdef DEBUG_DISASM
      debug_log(DEBUG_DETAIL, "[DASM] %04X:%04X, opcode: %02X\t\t", cpu->segregs[regcs], cpu->ip, cpu->opcode);
      debug_log(DEBUG_DETAIL, "dec %s\n", cpuRegNamesChar[tmpReg]);
#endif
      break;
    }
    case 0x37: {    // 37 AAA ASCII
      if (((cpu->regs.byteregs[regal] & 0xF) > 9) || (cpu->af == 1)) {
        cpu->regs.wordregs[regax] = cpu->regs.wordregs[regax] + 0x106;
        cpu->af = 1;
        cpu->cf = 1;
      } else {
        cpu->af = 0;
        cpu->cf = 0;
      }

      cpu->regs.byteregs[regal] = cpu->regs.byteregs[regal] & 0xF;
      loopcount +=4;
#ifdef DEBUG_DISASM
      debug_log(DEBUG_DETAIL, "[DASM] [not implemented] %04X:%04X, opcode: %02X\tAAA\r\n", cpu->segregs[regcs], cpu->ip, cpu->opcode);
#endif
      break;
    }
    case 0x27: {    // 27 DAA (BAA?)
#ifdef DEBUG_DISASM
      debug_log(DEBUG_DETAIL, "[DASM] [not implemented] %04X:%04X, opcode: %02X\tdaa\r\n", cpu->segregs[regcs], cpu->ip, cpu->opcode);
#endif

      uint8_t old_al_DAA;
      old_al_DAA = cpu->regs.byteregs[regal];
      if (((cpu->regs.byteregs[regal] & 0x0F) > 9) || cpu->af) {
        cpu->oper1 = (uint16_t)cpu->regs.byteregs[regal] + 0x06;
        cpu->regs.byteregs[regal] = cpu->oper1 & 0xFF;
        if (cpu->oper1 & 0xFF00) {
          cpu->cf = 1;
        }
        if ((cpu->oper1 & 0x000F) < (old_al_DAA & 0x0F)) {
          cpu->af = 1;
        }
      }
      if (((cpu->regs.byteregs[regal] & 0xF0) > 0x90) || cpu->cf) {
        cpu->oper1 = (uint16_t)cpu->regs.byteregs[regal] + 0x60;
        cpu->regs.byteregs[regal] = cpu->oper1 & 0xFF;
        if (cpu->oper1 & 0xFF00) {
          cpu->cf = 1;
        } else {
          cpu->cf = 0;
        }
      }
      flag_szp8(cpu, cpu->regs.byteregs[regal]);
      loopcount +=4;
      break;
    }
    case 0x28: {    // 28 SUB Eb Gb
      modregrm(cpu);
      cpu->oper1b = readrm8(cpu, cpu->rm);
      cpu->oper2b = getreg8(cpu, cpu->reg);
      op_sub8(cpu);
      writerm8(cpu, cpu->rm, cpu->res8);
      loopcount +=3;
#ifdef DEBUG_DISASM
      debug_log(DEBUG_DETAIL, "[DASM] [not implemented] %04X:%04X, opcode: %02X\tsub\r\n", cpu->segregs[regcs], cpu->ip, cpu->opcode);
#endif
      break;
    }
    case 0x29: {    // 29 SUB Ev Gv
      modregrm(cpu);
      cpu->oper1 = readrm16(cpu, cpu->rm);
      cpu->oper2 = getreg16(cpu, cpu->reg);
      op_sub16(cpu);
      writerm16(cpu, cpu->rm, cpu->res16);
      loopcount +=3;
#ifdef DEBUG_DISASM
      debug_log(DEBUG_DETAIL, "[DASM] [not implemented] %04X:%04X, opcode: %02X\tsub\r\n", cpu->segregs[regcs], cpu->ip, cpu->opcode);
#endif
      break;
    }
    case 0x2A: {    // 2A SUB Gb Eb
      modregrm(cpu);
      cpu->oper1b = getreg8(cpu, cpu->reg);
      cpu->oper2b = readrm8(cpu, cpu->rm);
      op_sub8(cpu);
      putreg8(cpu, cpu->reg, cpu->res8);
      loopcount +=3;
#ifdef DEBUG_DISASM
      debug_log(DEBUG_DETAIL, "[DASM] [not implemented] %04X:%04X, opcode: %02X\tsub\r\n", cpu->segregs[regcs], cpu->ip, cpu->opcode);
#endif
      break;
    }
    case 0x2B: {    // 2B SUB Gv Ev
      modregrm(cpu);
      cpu->oper1 = getreg16(cpu, cpu->reg);
      cpu->oper2 = readrm16(cpu, cpu->rm);
      op_sub16(cpu);
      putreg16(cpu, cpu->reg, cpu->res16);
      loopcount +=3;
#ifdef DEBUG_DISASM
      debug_log(DEBUG_DETAIL, "[DASM] [not implemented] %04X:%04X, opcode: %02X\tsub\r\n", cpu->segregs[regcs], cpu->ip, cpu->opcode);
#endif
      break;
    }
    case 0x2C: {    // 2C SUB cpu->regs.byteregs[regal] Ib
      cpu->oper1b = cpu->regs.byteregs[regal];
      cpu->oper2b = getmem8(cpu, cpu->segregs[regcs], cpu->ip);
      StepIP(cpu, 1);
      op_sub8(cpu);
      cpu->regs.byteregs[regal] = cpu->res8;
      loopcount +=3;
#ifdef DEBUG_DISASM
      debug_log(DEBUG_DETAIL, "[DASM] [not implemented] %04X:%04X, opcode: %02X\tsub\r\n", cpu->segregs[regcs], cpu->ip, cpu->opcode);
#endif
      break;
    }
    case 0x2D: {    // 2D SUB eAX Iv
      cpu->oper1 = cpu->regs.wordregs[regax];
      cpu->oper2 = getmem16(cpu, cpu->segregs[regcs], cpu->ip);
      StepIP(cpu, 2);
      op_sub16(cpu);
      cpu->regs.wordregs[regax] = cpu->res16;
      loopcount +=3;
#ifdef DEBUG_DISASM
      debug_log(DEBUG_DETAIL, "[DASM] [not implemented] %04X:%04X, opcode: %02X\tsub\r\n", cpu->segregs[regcs], cpu->ip, cpu->opcode);
#endif
      break;
    }
    case 0x18: {    // 18 SBB Eb Gb
      modregrm(cpu);
      cpu->oper1b = readrm8(cpu, cpu->rm);
      cpu->oper2b = getreg8(cpu, cpu->reg);
      op_sbb8(cpu);
      writerm8(cpu, cpu->rm, cpu->res8);
      loopcount +=3;
#ifdef DEBUG_DISASM
      debug_log(DEBUG_DETAIL, "[DASM] [not implemented] %04X:%04X, opcode: %02X\tsbb\r\n", cpu->segregs[regcs], cpu->ip, cpu->opcode);
#endif
      break;
    }
    case 0x19: {    // 19 SBB Ev Gv
      modregrm(cpu);
      cpu->oper1 = readrm16(cpu, cpu->rm);
      cpu->oper2 = getreg16(cpu, cpu->reg);
      op_sbb16(cpu);
      writerm16(cpu, cpu->rm, cpu->res16);
      loopcount +=3;
#ifdef DEBUG_DISASM
      debug_log(DEBUG_DETAIL, "[DASM] [not implemented] %04X:%04X, opcode: %02X\tsbb\r\n", cpu->segregs[regcs], cpu->ip, cpu->opcode);
#endif
      break;
    }
    case 0x1A: {    // 1A SBB Gb Eb
      modregrm(cpu);
      cpu->oper1b = getreg8(cpu, cpu->reg);
      cpu->oper2b = readrm8(cpu, cpu->rm);
      op_sbb8(cpu);
      putreg8(cpu, cpu->reg, cpu->res8);
      loopcount +=3;
#ifdef DEBUG_DISASM
      debug_log(DEBUG_DETAIL, "[DASM] [not implemented] %04X:%04X, opcode: %02X\tsbb\r\n", cpu->segregs[regcs], cpu->ip, cpu->opcode);
#endif
      break;
    }
    case 0x1B: {    // 1B SBB Gv Ev
      modregrm(cpu);
      cpu->oper1 = getreg16(cpu, cpu->reg);
      cpu->oper2 = readrm16(cpu, cpu->rm);
      op_sbb16(cpu);
      putreg16(cpu, cpu->reg, cpu->res16);
      loopcount +=3;
#ifdef DEBUG_DISASM
      debug_log(DEBUG_DETAIL, "[DASM] [not implemented] %04X:%04X, opcode: %02X\tsbb\r\n", cpu->segregs[regcs], cpu->ip, cpu->opcode);
#endif
      break;
    }
    case 0x1C: {    // 1C SBB cpu->regs.byteregs[regal] Ib
      cpu->oper1b = cpu->regs.byteregs[regal];
      cpu->oper2b = getmem8(cpu, cpu->segregs[regcs], cpu->ip);
      StepIP(cpu, 1);
      op_sbb8(cpu);
      cpu->regs.byteregs[regal] = cpu->res8;
      loopcount +=3;
#ifdef DEBUG_DISASM
      debug_log(DEBUG_DETAIL, "[DASM] [not implemented] %04X:%04X, opcode: %02X\tsbb\r\n", cpu->segregs[regcs], cpu->ip, cpu->opcode);
#endif
      break;
    }
    case 0x1D: {    // 1D SBB eAX Iv
      cpu->oper1 = cpu->regs.wordregs[regax];
      cpu->oper2 = getmem16(cpu, cpu->segregs[regcs], cpu->ip);
      StepIP(cpu, 2);
      op_sbb16(cpu);
      cpu->regs.wordregs[regax] = cpu->res16;
      loopcount +=3;
#ifdef DEBUG_DISASM
      debug_log(DEBUG_DETAIL, "[DASM] [not implemented] %04X:%04X, opcode: %02X\tsbb\r\n", cpu->segregs[regcs], cpu->ip, cpu->opcode);
#endif
      break;
    }
    case 0x38: {    // 38 CMP Eb Gb
      modregrm(cpu);
      cpu->oper1b = readrm8(cpu, cpu->rm);
      cpu->oper2b = getreg8(cpu, cpu->reg);
      flag_sub8(cpu, cpu->oper1b, cpu->oper2b);
      loopcount +=3;
#ifdef DEBUG_DISASM
      debug_log(DEBUG_DETAIL, "[DASM] [not implemented] %04X:%04X, opcode: %02X\tcmp\r\n", cpu->segregs[regcs], cpu->ip, cpu->opcode);
#endif
      break;
    }
    case 0x39: {    // 39 CMP Ev Gv
      modregrm(cpu);
      cpu->oper1 = readrm16(cpu, cpu->rm);
      cpu->oper2 = getreg16(cpu, cpu->reg);
      flag_sub16(cpu, cpu->oper1, cpu->oper2);
      loopcount +=3;
#ifdef DEBUG_DISASM
      debug_log(DEBUG_DETAIL, "[DASM] [not implemented] %04X:%04X, opcode: %02X\tcmp\r\n", cpu->segregs[regcs], cpu->ip, cpu->opcode);
#endif
      break;
    }
    case 0x3A: {    // 3A CMP Gb Eb
      modregrm(cpu);
      cpu->oper1b = getreg8(cpu, cpu->reg);
      cpu->oper2b = readrm8(cpu, cpu->rm);
      flag_sub8(cpu, cpu->oper1b, cpu->oper2b);
      loopcount +=3;
#ifdef DEBUG_DISASM
      debug_log(DEBUG_DETAIL, "[DASM] [not implemented] %04X:%04X, opcode: %02X\tcmp\r\n", cpu->segregs[regcs], cpu->ip, cpu->opcode);
#endif
      break;
    }
    case 0x3B: {    // 3B CMP Gv Ev
      modregrm(cpu);
      cpu->oper1 = getreg16(cpu, cpu->reg);
      cpu->oper2 = readrm16(cpu, cpu->rm);
      flag_sub16(cpu, cpu->oper1, cpu->oper2);
      loopcount +=3;
#ifdef DEBUG_DISASM
      debug_log(DEBUG_DETAIL, "[DASM] [not implemented] %04X:%04X, opcode: %02X\tcmp\r\n", cpu->segregs[regcs], cpu->ip, cpu->opcode);
#endif
      break;
    }
    case 0x3C: {    // 3C CMP cpu->regs.byteregs[regal] Ib
      cpu->oper1b = cpu->regs.byteregs[regal];
      cpu->oper2b = getmem8(cpu, cpu->segregs[regcs], cpu->ip);
      StepIP(cpu, 1);
      flag_sub8(cpu, cpu->oper1b, cpu->oper2b);
      loopcount +=3;
#ifdef DEBUG_DISASM
      debug_log(DEBUG_DETAIL, "[DASM] [not implemented] %04X:%04X, opcode: %02X\tcmp\r\n", cpu->segregs[regcs], cpu->ip, cpu->opcode);
#endif
      break;
    }
    case 0x3D: {    // 3D CMP eAX Iv
      cpu->oper1 = cpu->regs.wordregs[regax];
      cpu->oper2 = getmem16(cpu, cpu->segregs[regcs], cpu->ip);
      StepIP(cpu, 2);
      flag_sub16(cpu, cpu->oper1, cpu->oper2);
      loopcount +=3;
#ifdef DEBUG_DISASM
      debug_log(DEBUG_DETAIL, "[DASM] [not implemented] %04X:%04X, opcode: %02X\tcmp\r\n", cpu->segregs[regcs], cpu->ip, cpu->opcode);
#endif
      break;
    }
    case 0x3F: {    // 3F AAS ASCII
#ifdef DEBUG_DISASM
      debug_log(DEBUG_DETAIL, "[DASM] [not implemented] %04X:%04X, opcode: %02X\taas\r\n", cpu->segregs[regcs], cpu->ip, cpu->opcode);
#endif

      if (((cpu->regs.byteregs[regal] & 0xF) > 9) || (cpu->af == 1)) {
        cpu->regs.wordregs[regax] = cpu->regs.wordregs[regax] - 6;
        cpu->regs.byteregs[regah] = cpu->regs.byteregs[regah] - 1;
        cpu->af = 1;
        cpu->cf = 1;
      } else {
        cpu->af = 0;
        cpu->cf = 0;
      }

      cpu->regs.byteregs[regal] = cpu->regs.byteregs[regal] & 0xF;
      loopcount +=4;
      break;
    }
    case 0x2F: {    // 2F DAS
#ifdef DEBUG_DISASM
      debug_log(DEBUG_DETAIL, "[DASM] [not implemented] %04X:%04X, opcode: %02X\tdas\r\n", cpu->segregs[regcs], cpu->ip, cpu->opcode);
#endif
      uint8_t old_al_DAS;
      old_al_DAS = cpu->regs.byteregs[regal];
      if (((cpu->regs.byteregs[regal] & 0x0F) > 9) || cpu->af) {
        cpu->oper1 = (uint16_t)cpu->regs.byteregs[regal] - 0x06;
        cpu->regs.byteregs[regal] = cpu->oper1 & 0xFF;
        if (cpu->oper1 & 0xFF00) {
          cpu->cf = 1;
        }
        if ((cpu->oper1 & 0x000F) >= (old_al_DAS & 0x0F)) {
          cpu->af = 1;
        }
      }
      if (((cpu->regs.byteregs[regal] & 0xF0) > 0x90) || cpu->cf) {
        cpu->oper1 = (uint16_t)cpu->regs.byteregs[regal] - 0x60;
        cpu->regs.byteregs[regal] = cpu->oper1 & 0xFF;
        if (cpu->oper1 & 0xFF00) {
          cpu->cf = 1;
        } else {
          cpu->cf = 0;
        }
      }
      flag_szp8(cpu, cpu->regs.byteregs[regal]);
      loopcount +=4;
      break;
    }
    case 0xD4: {    // D4 AAM I0
#ifdef DEBUG_DISASM
      debug_log(DEBUG_DETAIL, "[DASM] [no implemented] %04X:%04X opcode: %02X\taam\r\n", cpu->segregs[regcs], cpu->ip, cpu->opcode);
#endif
      cpu->oper1 = getmem8(cpu, cpu->segregs[regcs], cpu->ip);
      StepIP(cpu, 1);
      if (!cpu->oper1) {
        cpu_intcall(cpu, 0);
        break;
      }       // division by zero

      cpu->regs.byteregs[regah] = (cpu->regs.byteregs[regal] / cpu->oper1) & 255;
      cpu->regs.byteregs[regal] = (cpu->regs.byteregs[regal] % cpu->oper1) & 255;
      flag_szp16(cpu, cpu->regs.wordregs[regax]);
      loopcount +=83;
      break;
    }
    case 0xD5: {    // D5 AAD I0
#ifdef DEBUG_DISASM
      debug_log(DEBUG_DETAIL, "[DASM] [no implemented] %04X:%04X opcode: %02X\taad\r\n", cpu->segregs[regcs], cpu->ip, cpu->opcode);
#endif

      cpu->oper1 = getmem8(cpu, cpu->segregs[regcs], cpu->ip);
      StepIP(cpu, 1);
      cpu->regs.byteregs[regal] = (cpu->regs.byteregs[regah] * cpu->oper1 + cpu->regs.byteregs[regal]) & 255;
      cpu->regs.byteregs[regah] = 0;
      flag_szp16(cpu, cpu->regs.byteregs[regah] * cpu->oper1 + cpu->regs.byteregs[regal]);
      cpu->sf = 0;
      loopcount +=60;
      break;
    }
    case 0x98: {    // 98 CBW
#ifdef DEBUG_DISASM
      debug_log(DEBUG_DETAIL, "[DASM] [not implemented] %04X:%04X, opcode: %02X\tcbw\r\n", cpu->segregs[regcs], cpu->ip, cpu->opcode);
#endif
      if ((cpu->regs.byteregs[regal] & 0x80) == 0x80) {
        cpu->regs.byteregs[regah] = 0xFF;
      } else {
        cpu->regs.byteregs[regah] = 0;
      }
      loopcount +=2;
      break;
    }
    case 0x99: {    // 99 CWD
#ifdef DEBUG_DISASM
      debug_log(DEBUG_DETAIL, "[DASM] [not implemented] %04X:%04X, opcode: %02X\tcwd\r\n", cpu->segregs[regcs], cpu->ip, cpu->opcode);
#endif
      if ((cpu->regs.byteregs[regah] & 0x80) == 0x80) {
        cpu->regs.wordregs[regdx] = 0xFFFF;
      } else {
        cpu->regs.wordregs[regdx] = 0;
      }
      loopcount +=5;
      break;
    }

    // Data transfer
    case 0x88: {    // 88 MOV Eb Gb
#ifdef DEBUG_DISASM
      debug_log(DEBUG_DETAIL, "[DASM] [not implemented] %04X:%04X, opcode: %02X\tmov\r\n", cpu->segregs[regcs], cpu->ip, cpu->opcode);
#endif
      modregrm(cpu);
      writerm8(cpu, cpu->rm, getreg8(cpu, cpu->reg));
      loopcount +=9;
      break;
    }
    case 0x89: {    // 89 MOV Ev Gv
#ifdef DEBUG_DISASM
      debug_log(DEBUG_DETAIL, "[DASM] [not implemented] %04X:%04X, opcode: %02X\tmov\r\n", cpu->segregs[regcs], cpu->ip, cpu->opcode);
#endif
      modregrm(cpu);
      writerm16(cpu, cpu->rm, getreg16(cpu, cpu->reg));
      loopcount +=9;
      break;
    }
    case 0x8A: {    // 8A MOV Gb Eb
#ifdef DEBUG_DISASM
      debug_log(DEBUG_DETAIL, "[DASM] [not implemented] %04X:%04X, opcode: %02X\tmov\r\n", cpu->segregs[regcs], cpu->ip, cpu->opcode);
#endif
      modregrm(cpu);
      putreg8(cpu, cpu->reg, readrm8(cpu, cpu->rm));
      loopcount +=2;
      break;
    }
    case 0x8B: {    // 8B MOV Gv Ev
      modregrm(cpu);
      putreg16(cpu, cpu->reg, readrm16(cpu, cpu->rm));
      loopcount +=2;
#ifdef DEBUG_DISASM
      debug_log(DEBUG_DETAIL, "[not implemented] mov %s, ???\n", cpuRegNamesChar[cpu->reg]);
#endif
      break;
    }
    case 0x8C: {    // 8C MOV Ew Sw
#ifdef DEBUG_DISASM
      debug_log(DEBUG_DETAIL, "[DASM] [not implemented] %04X:%04X, opcode: %02X\tmov\r\n", cpu->segregs[regcs], cpu->ip, cpu->opcode);
#endif
      modregrm(cpu);
      writerm16(cpu, cpu->rm, getsegreg(cpu, cpu->reg));
      loopcount +=9;
      break;
    }
    case 0x8E: {    // 8E MOV Sw Ew
      modregrm(cpu);
      putsegreg(cpu, cpu->reg, readrm16(cpu, cpu->rm));

#ifdef DEBUG_DISASM
      debug_log(DEBUG_DETAIL, "[DASM] [new] [testing] %04X:%04X, op: %02X\t\t", cpu->savecs, cpu->saveip, cpu->opcode);
      debug_log(DEBUG_DETAIL, "mov %s, ", cpuRegNamesChar[cpu->reg + 8]);
      if (cpu->mode < 3) {
      debug_log(DEBUG_DETAIL, "%05X\n", cpu->ea);
      } else {
        debug_log(DEBUG_DETAIL, "%s\n", cpuRegNamesChar[cpu->rm]);
      }
#endif
      loopcount +=2;
      break;
    }
    case 0xA0: {    // A0 MOV cpu->regs.byteregs[regal] Ob
#ifdef DEBUG_DISASM
      debug_log(DEBUG_DETAIL, "[DASM] [not implemented] %04X:%04X, opcode: %02X\tmov\r\n", cpu->segregs[regcs], cpu->ip, cpu->opcode);
#endif
      cpu->regs.byteregs[regal] = getmem8(cpu, cpu->useseg, getmem16(cpu, cpu->segregs[regcs], cpu->ip));
      StepIP(cpu, 2);
      loopcount +=9;
      break;
    }
    case 0xA1: {    // A1 MOV eAX Ov
#ifdef DEBUG_DISASM
      debug_log(DEBUG_DETAIL, "[DASM] [not implemented] %04X:%04X, opcode: %02X\tmov\r\n", cpu->segregs[regcs], cpu->ip, cpu->opcode);
#endif
      cpu->oper1 = getmem16(cpu, cpu->useseg, getmem16(cpu, cpu->segregs[regcs], cpu->ip));
      StepIP(cpu, 2);
      cpu->regs.wordregs[regax] = cpu->oper1;
      loopcount +=9;
      break;
    }
    case 0xA2: {    // A2 MOV Ob cpu->regs.byteregs[regal]
#ifdef DEBUG_DISASM
      debug_log(DEBUG_DETAIL, "[DASM] [not implemented] %04X:%04X, opcode: %02X\tmov\r\n", cpu->segregs[regcs], cpu->ip, cpu->opcode);
#endif
      putmem8(cpu, cpu->useseg, getmem16(cpu, cpu->segregs[regcs], cpu->ip), cpu->regs.byteregs[regal]);
      StepIP(cpu, 2);
      loopcount +=8;
      break;
    }
    case 0xA3: {    // A3 MOV Ov eAX
#ifdef DEBUG_DISASM
      debug_log(DEBUG_DETAIL, "[DASM] [not implemented] %04X:%04X, opcode: %02X\tmov\r\n", cpu->segregs[regcs], cpu->ip, cpu->opcode);
#endif
      putmem16(cpu, cpu->useseg, getmem16(cpu, cpu->segregs[regcs], cpu->ip), cpu->regs.wordregs[regax]);
      StepIP(cpu, 2);
      loopcount +=8;
      break;
    }
    case 0xB0: {    // B0 MOV al, Immediate byte
      cpu->regs.byteregs[regal] = getmem8(cpu, cpu->segregs[regcs], cpu->ip);
      StepIP(cpu, 1);
      loopcount +=4;
#ifdef DEBUG_DISASM
      debug_log(DEBUG_DETAIL, "mov al, %02Xh\n", cpu->regs.byteregs[regal]);
#endif
      break;
    }
    case 0xB1: {    // B1 MOV cpu->regs.byteregs[regcl] Ib
#ifdef DEBUG_DISASM
      debug_log(DEBUG_DETAIL, "[DASM] [not implemented] %04X:%04X, opcode: %02X\tmov\r\n", cpu->segregs[regcs], cpu->ip, cpu->opcode);
#endif
      cpu->regs.byteregs[regcl] = getmem8(cpu, cpu->segregs[regcs], cpu->ip);
      StepIP(cpu, 1);
      loopcount +=4;
      break;
    }
    case 0xB2: {    // B2 MOV cpu->regs.byteregs[regdl] Ib
#ifdef DEBUG_DISASM
      debug_log(DEBUG_DETAIL, "[DASM] [not implemented] %04X:%04X, opcode: %02X\tmov\r\n", cpu->segregs[regcs], cpu->ip, cpu->opcode);
#endif
      cpu->regs.byteregs[regdl] = getmem8(cpu, cpu->segregs[regcs], cpu->ip);
      StepIP(cpu, 1);
      loopcount +=4;
      break;
    }
    case 0xB3: {    // B3 MOV cpu->regs.byteregs[regbl] Ib
#ifdef DEBUG_DISASM
      debug_log(DEBUG_DETAIL, "[DASM] [not implemented] %04X:%04X, opcode: %02X\tmov\r\n", cpu->segregs[regcs], cpu->ip, cpu->opcode);
#endif
      cpu->regs.byteregs[regbl] = getmem8(cpu, cpu->segregs[regcs], cpu->ip);
      StepIP(cpu, 1);
      loopcount +=4;
      break;
    }
    case 0xB4: {    // B4 MOV cpu->regs.byteregs[regah] Ib
#ifdef DEBUG_DISASM
      debug_log(DEBUG_DETAIL, "[DASM] [not implemented] %04X:%04X, opcode: %02X\tmov\r\n", cpu->segregs[regcs], cpu->ip, cpu->opcode);
#endif
      cpu->regs.byteregs[regah] = getmem8(cpu, cpu->segregs[regcs], cpu->ip);
      StepIP(cpu, 1);
      loopcount +=4;
      break;
    }
    case 0xB5: {    // B5 MOV cpu->regs.byteregs[regch] Ib
#ifdef DEBUG_DISASM
      debug_log(DEBUG_DETAIL, "[DASM] [not implemented] %04X:%04X, opcode: %02X\tmov\r\n", cpu->segregs[regcs], cpu->ip, cpu->opcode);
#endif
      cpu->regs.byteregs[regch] = getmem8(cpu, cpu->segregs[regcs], cpu->ip);
      StepIP(cpu, 1);
      loopcount +=4;
      break;
    }
    case 0xB6: {    // B6 MOV cpu->regs.byteregs[regdh] Ib
#ifdef DEBUG_DISASM
      debug_log(DEBUG_DETAIL, "[DASM] [not implemented] %04X:%04X, opcode: %02X\tmov\r\n", cpu->segregs[regcs], cpu->ip, cpu->opcode);
#endif
      cpu->regs.byteregs[regdh] = getmem8(cpu, cpu->segregs[regcs], cpu->ip);
      StepIP(cpu, 1);
      loopcount +=4;
      break;
    }
    case 0xB7: {    // B7 MOV cpu->regs.byteregs[regbh] Ib
#ifdef DEBUG_DISASM
      debug_log(DEBUG_DETAIL, "[DASM] [not implemented] %04X:%04X, opcode: %02X\tmov\r\n", cpu->segregs[regcs], cpu->ip, cpu->opcode);
#endif
      cpu->regs.byteregs[regbh] = getmem8(cpu, cpu->segregs[regcs], cpu->ip);
      StepIP(cpu, 1);
      loopcount +=4;
      break;
    }
    case 0xB8 ... 0xBF: {    // MOV 16 bit registers ax cx dx bx sp bp si di, 16 bit Immediate
      uint8_t tmpReg;
      tmpReg = cpu->opcode & 0x7;
      cpu->regs.wordregs[tmpReg] = getmem16(cpu, cpu->segregs[regcs], cpu->ip);
      StepIP(cpu, 2);
      loopcount +=4;
#ifdef DEBUG_DISASM
      debug_log(DEBUG_DETAIL, "[DASM] [new] %04X:%04X, opcode: %02X\t\t", cpu->savecs, cpu->saveip, cpu->opcode);
      debug_log(DEBUG_DETAIL, "[testing] mov %s, %04Xh\n", cpuRegNamesChar[tmpReg], cpu->regs.wordregs[tmpReg]);
#endif
      break;
    }
    case 0xC6: {    // C6 MOV Eb Ib
#ifdef DEBUG_DISASM
      debug_log(DEBUG_DETAIL, "[DASM] [not implemented] %04X:%04X, opcode: %02X\tmov\r\n", cpu->segregs[regcs], cpu->ip, cpu->opcode);
#endif
      modregrm(cpu);
      writerm8(cpu, cpu->rm, getmem8(cpu, cpu->segregs[regcs], cpu->ip));
      StepIP(cpu, 1);
      loopcount +=10;
      break;
    }
    case 0xC7: {    // C7 MOV Ev Iv
      modregrm(cpu);
      writerm16(cpu, cpu->rm, getmem16(cpu, cpu->segregs[regcs], cpu->ip));
      StepIP(cpu, 2);
      loopcount +=10;
#ifdef DEBUG_DISASM
      debug_log(DEBUG_DETAIL, "[not implemented] mov ???, ???\n");
#endif
      break;
    }
    case 0x6:  {    // 06 PUSH cpu->segregs[reges]
      push(cpu, cpu->segregs[reges]);
      loopcount +=10;
#ifdef DEBUG_DISASM
      debug_log(DEBUG_DETAIL, "[DASM] %04X:%04X, opcode: %02X\tpush es\r\n", cpu->segregs[regcs], cpu->ip, cpu->opcode);
#endif
      break;
    }
    case 0xE:  {    // 0E PUSH cpu->segregs[regcs]
#ifdef DEBUG_DISASM
      debug_log(DEBUG_DETAIL, "[DASM] %04X:%04X, opcode: %02X\tpush cs\r\n", cpu->segregs[regcs], cpu->ip, cpu->opcode);
#endif
      push(cpu, cpu->segregs[regcs]);
      loopcount +=10;
      break;
    }
    case 0x16: {    // 16 PUSH cpu->segregs[regss]
#ifdef DEBUG_DISASM
      debug_log(DEBUG_DETAIL, "[DASM] %04X:%04X, opcode: %02X\tpush ss\r\n", cpu->segregs[regcs], cpu->ip, cpu->opcode);
#endif
      push(cpu, cpu->segregs[regss]);
      loopcount +=10;
      break;
    }
    case 0x1E: {    // 1E PUSH cpu->segregs[regds]
#ifdef DEBUG_DISASM
      debug_log(DEBUG_DETAIL, "[DASM] %04X:%04X, opcode: %02X\tpush ds\r\n", cpu->segregs[regcs], cpu->ip, cpu->opcode);
#endif
      push(cpu, cpu->segregs[regds]);
      loopcount +=10;
      break;
    }
    case 0x50 ... 0x57: {    // PUSH 16 bit registers ax cx dx bx sp bp si di
      uint8_t tmpReg;
      tmpReg = cpu->opcode & 0x7;
      push(cpu, cpu->regs.wordregs[tmpReg]);
      loopcount +=10;
#ifdef DEBUG_DISASM
      debug_log(DEBUG_DETAIL, "[DASM] [new] %04X:%04X, opcode: %02X\t\t", cpu->savecs, cpu->saveip, cpu->opcode);
      debug_log(DEBUG_DETAIL, "push %s\n", cpuRegNamesChar[tmpReg]);
#endif
      break;
    }
    case 0x7:  {    // 07 POP cpu->segregs[reges]
#ifdef DEBUG_DISASM
      debug_log(DEBUG_DETAIL, "[DASM] %04X:%04X, opcode: %02X\tpop es\r\n", cpu->segregs[regcs], cpu->ip, cpu->opcode);
#endif
      cpu->segregs[reges] = pop(cpu);
      loopcount +=8;
      break;
    }
    case 0xF:  {    // 0F POP CS //only the 8086/8088 does this.
#ifdef DEBUG_DISASM
      debug_log(DEBUG_DETAIL, "[DASM] %04X:%04X, opcode: %02X\tpop cs\r\n", cpu->segregs[regcs], cpu->ip, cpu->opcode);
#endif
      cpu->segregs[regcs] = pop(cpu);
      loopcount +=8;
      break;
    }
    case 0x17: {    // 17 POP cpu->segregs[regss]
#ifdef DEBUG_DISASM
      debug_log(DEBUG_DETAIL, "[DASM] %04X:%04X, opcode: %02X\tpop ss\r\n", cpu->segregs[regcs], cpu->ip, cpu->opcode);
#endif
      cpu->segregs[regss] = pop(cpu);
      loopcount +=8;
      break;
    }
    case 0x1F: {    // 1F POP cpu->segregs[regds]
#ifdef DEBUG_DISASM
      debug_log(DEBUG_DETAIL, "[DASM] %04X:%04X, opcode: %02X\tpop ds\r\n", cpu->segregs[regcs], cpu->ip, cpu->opcode);
#endif
      cpu->segregs[regds] = pop(cpu);
      loopcount +=10;
      break;
    }
    case 0x58 ... 0x5F: {    // POP 16 bit registers ax cx dx bx sp bp si di
      uint8_t tmpReg;
      tmpReg = cpu->opcode & 0x7;
      cpu->regs.wordregs[tmpReg] = pop(cpu);
      loopcount +=8;
#ifdef DEBUG_DISASM
      debug_log(DEBUG_DETAIL, "[DASM] [new] %04X:%04X, opcode: %02X\t\t", cpu->savecs, cpu->saveip, cpu->opcode);
      debug_log(DEBUG_DETAIL, "pop %s\n", cpuRegNamesChar[tmpReg]);
#endif
      break;
    }
    case 0x8F: {    // 8F POP Ev
#ifdef DEBUG_DISASM
      debug_log(DEBUG_DETAIL, "[DASM] [not implemented] %04X:%04X, opcode: %02X\tpop\r\n", cpu->segregs[regcs], cpu->ip, cpu->opcode);
#endif
      modregrm(cpu);
      writerm16(cpu, cpu->rm, pop(cpu));
      loopcount +=17;
      break;
    }
    case 0x86: {    // 86 XCHG Gb Eb
      modregrm(cpu);
      cpu->oper1b = getreg8(cpu, cpu->reg);
      putreg8(cpu, cpu->reg, readrm8(cpu, cpu->rm));
      writerm8(cpu, cpu->rm, cpu->oper1b);
      loopcount +=3;
#ifdef DEBUG_DISASM
      debug_log(DEBUG_DETAIL, "[DASM] [not implemented] %04X:%04X, opcode: %02X\txchg\r\n", cpu->segregs[regcs], cpu->ip, cpu->opcode);
#endif
      break;
    }
    case 0x87: {    // 87 XCHG Gv Ev
#ifdef DEBUG_DISASM
      debug_log(DEBUG_DETAIL, "[DASM] [not implemented] %04X:%04X, opcode: %02X\txchg\r\n", cpu->segregs[regcs], cpu->ip, cpu->opcode);
#endif
      modregrm(cpu);
      cpu->oper1 = getreg16(cpu, cpu->reg);
      putreg16(cpu, cpu->reg, readrm16(cpu, cpu->rm));
      writerm16(cpu, cpu->rm, cpu->oper1);
      loopcount +=3;
      break;
    }
    case 0x91 ... 0x97: {    // XCHG 16 bit registers cx dx bx sp bp si di with ax
      uint8_t tmpReg;
      tmpReg = cpu->opcode & 0x7;
      cpu->oper1 = cpu->regs.wordregs[tmpReg];
      cpu->regs.wordregs[tmpReg] = cpu->regs.wordregs[regax];
      cpu->regs.wordregs[regax] = cpu->oper1;
#ifdef DEBUG_DISASM
      debug_log(DEBUG_DETAIL, "[DASM] [new] [testing] %04X:%04X, opcode: %02X\t\t", cpu->savecs, cpu->saveip, cpu->opcode);
      debug_log(DEBUG_DETAIL, "xchg %s, ax\n", cpuRegNamesChar[tmpReg]);
#endif

      loopcount +=4;
      break;
    }
    case 0xE4: {    // E4 IN cpu->regs.byteregs[regal] Ib
#ifdef DEBUG_DISASM
      debug_log(DEBUG_DETAIL, "[DASM] [not implemented] %04X:%04X, opcode: %02X\tin\r\n", cpu->segregs[regcs], cpu->ip, cpu->opcode);
#endif
      cpu->oper1b = getmem8(cpu, cpu->segregs[regcs], cpu->ip);
      StepIP(cpu, 1);
      cpu->regs.byteregs[regal] = (uint8_t)port_read(cpu, cpu->oper1b);
      loopcount +=10;
      break;
    }
    case 0xE5: {    // E5 IN eAX Ib
#ifdef DEBUG_DISASM
      debug_log(DEBUG_DETAIL, "[DASM] [not implemented] %04X:%04X, opcode: %02X\tin\r\n", cpu->segregs[regcs], cpu->ip, cpu->opcode);
#endif
      cpu->oper1b = getmem8(cpu, cpu->segregs[regcs], cpu->ip);
      StepIP(cpu, 1);
      cpu->regs.wordregs[regax] = port_readw(cpu, cpu->oper1b);
      loopcount +=10;
      break;
    }
    case 0xEC: {    // EC IN cpu->regs.byteregs[regal] regdx
#ifdef DEBUG_DISASM
      debug_log(DEBUG_DETAIL, "[DASM] [not implemented] %04X:%04X, opcode: %02X\tin\r\n", cpu->segregs[regcs], cpu->ip, cpu->opcode);
#endif
      cpu->oper1 = cpu->regs.wordregs[regdx];
      cpu->regs.byteregs[regal] = (uint8_t)port_read(cpu, cpu->oper1);
      loopcount +=8;
      break;
    }
    case 0xED: {    // ED IN eAX regdx
#ifdef DEBUG_DISASM
      debug_log(DEBUG_DETAIL, "[DASM] [not implemented] %04X:%04X, opcode: %02X\tin\r\n", cpu->segregs[regcs], cpu->ip, cpu->opcode);
#endif
      cpu->oper1 = cpu->regs.wordregs[regdx];
      cpu->regs.wordregs[regax] = port_readw(cpu, cpu->oper1);
      loopcount +=8;
      break;
    }
    case 0xE6: {    // E6 OUT Ib cpu->regs.byteregs[regal]
#ifdef DEBUG_DISASM
      debug_log(DEBUG_DETAIL, "[DASM] [not implemented] %04X:%04X, opcode: %02X\tout\r\n", cpu->segregs[regcs], cpu->ip, cpu->opcode);
#endif
      cpu->oper1b = getmem8(cpu, cpu->segregs[regcs], cpu->ip);
      StepIP(cpu, 1);
      port_write(cpu, cpu->oper1b, cpu->regs.byteregs[regal]);
      loopcount +=10;
      break;
    }
    case 0xE7: {    // E7 OUT Ib eAX
#ifdef DEBUG_DISASM
      debug_log(DEBUG_DETAIL, "[DASM] [not implemented] %04X:%04X, opcode: %02X\tout\r\n", cpu->segregs[regcs], cpu->ip, cpu->opcode);
#endif
      cpu->oper1b = getmem8(cpu, cpu->segregs[regcs], cpu->ip);
      StepIP(cpu, 1);
      port_writew(cpu, cpu->oper1b, cpu->regs.wordregs[regax]);
      loopcount +=10;
      break;
    }
    case 0xEE: {    // EE OUT regdx cpu->regs.byteregs[regal]
#ifdef DEBUG_DISASM
      debug_log(DEBUG_DETAIL, "[DASM] [not implemented] %04X:%04X, opcode: %02X\tout\r\n", cpu->segregs[regcs], cpu->ip, cpu->opcode);
#endif
      cpu->oper1 = cpu->regs.wordregs[regdx];
      port_write(cpu, cpu->oper1, cpu->regs.byteregs[regal]);
      loopcount +=8;
      break;
    }
    case 0xEF: {    // EF OUT regdx eAX
#ifdef DEBUG_DISASM
      debug_log(DEBUG_DETAIL, "[DASM] [not implemented] %04X:%04X, opcode: %02X\tout\r\n", cpu->segregs[regcs], cpu->ip, cpu->opcode);
#endif
      cpu->oper1 = cpu->regs.wordregs[regdx];
      port_writew(cpu, cpu->oper1, cpu->regs.wordregs[regax]);
      loopcount +=8;
      break;
    }
    case 0xD7: {    // D7 XLAT
#ifdef DEBUG_DISASM
      debug_log(DEBUG_DETAIL, "[DASM] [no implemented] %04X:%04X opcode: %02X\txlat\r\n", cpu->segregs[regcs], cpu->ip, cpu->opcode);
#endif
      cpu->regs.byteregs[regal] = cpu_read(cpu, cpu->useseg * 16 + (cpu->regs.wordregs[regbx]) + cpu->regs.byteregs[regal]);
      loopcount +=11;
      break;
    }
    case 0x8D: {    // 8D LEA Gv M
#ifdef DEBUG_DISASM
      debug_log(DEBUG_DETAIL, "[DASM] [not implemented] %04X:%04X, opcode: %02X\tlea\r\n", cpu->segregs[regcs], cpu->ip, cpu->opcode);
#endif
      modregrm(cpu);
      getea(cpu, cpu->rm);
      putreg16(cpu, cpu->reg, cpu->ea - segbase(cpu->useseg));
      loopcount +=8;
      break;
    }
    case 0xC5: {    // C5 LDS Gv Mp
#ifdef DEBUG_DISASM
      debug_log(DEBUG_DETAIL, "[DASM] [not implemented] %04X:%04X, opcode: %02X\tlds\r\n", cpu->segregs[regcs], cpu->ip, cpu->opcode);
#endif
      modregrm(cpu);
      getea(cpu, cpu->rm);
      putreg16(cpu, cpu->reg, cpu_read(cpu, cpu->ea) + cpu_read(cpu, cpu->ea + 1) * 256);
      cpu->segregs[regds] = cpu_read(cpu, cpu->ea + 2) + cpu_read(cpu, cpu->ea + 3) * 256;
      loopcount +=16;
      break;
    }
    case 0xC4: {    // C4 LES Gv Mp
#ifdef DEBUG_DISASM
      debug_log(DEBUG_DETAIL, "[DASM] [not implemented] %04X:%04X, opcode: %02X\tles\r\n", cpu->segregs[regcs], cpu->ip, cpu->opcode);
#endif
      modregrm(cpu);
      getea(cpu, cpu->rm);
      putreg16(cpu, cpu->reg, cpu_read(cpu, cpu->ea) + cpu_read(cpu, cpu->ea + 1) * 256);
      cpu->segregs[reges] = cpu_read(cpu, cpu->ea + 2) + cpu_read(cpu, cpu->ea + 3) * 256;
      loopcount +=16;
      break;
    }
    case 0x9F: {    // 9F LAHF
#ifdef DEBUG_DISASM
      debug_log(DEBUG_DETAIL, "[DASM] [not implemented] %04X:%04X, opcode: %02X\tlahf\r\n", cpu->segregs[regcs], cpu->ip, cpu->opcode);
#endif
      cpu->regs.byteregs[regah] = makeflagsword(cpu) & 0xFF;
      loopcount +=4;
      break;
    }
    case 0x9E: {    // 9E SAHF
#ifdef DEBUG_DISASM
      debug_log(DEBUG_DETAIL, "[DASM] [not implemented] %04X:%04X, opcode: %02X\tsahf\r\n", cpu->segregs[regcs], cpu->ip, cpu->opcode);
#endif
      decodeflagsword(cpu, (makeflagsword(cpu) & 0xFF00) | cpu->regs.byteregs[regah]);
      loopcount +=4;
      break;
    }
    case 0x9C: {    // 9C PUSHF
#ifdef DEBUG_DISASM
      debug_log(DEBUG_DETAIL, "[DASM] %04X:%04X, opcode: %02X\tpushf\r\n", cpu->segregs[regcs], cpu->ip, cpu->opcode);
#endif

      //#ifdef CPU_SET_HIGH_FLAGS
      //push(cpu, makeflagsword(cpu) | 0xF800);
      //loopcount +=10;
      //#else
      push(cpu, makeflagsword(cpu) | 0x0800);
      //#endif
      break;
    }
    case 0x9D: {    // 9D POPF
#ifdef DEBUG_DISASM
      debug_log(DEBUG_DETAIL, "[DASM] %04X:%04X, opcode: %02X\tpopf\r\n", cpu->segregs[regcs], cpu->ip, cpu->opcode);
#endif
      cpu->temp16 = pop(cpu);
      decodeflagsword(cpu, cpu->temp16);
      loopcount +=8;
      break;
    }

    // Misc. unsorted
    case 0xF6: {    // F6 GRP3a Eb
#ifdef DEBUG_DISASM
      debug_log(DEBUG_DETAIL, "[DASM] [not implemented] %04X:%04X, opcode: %02X\t", cpu->segregs[regcs], cpu->ip, cpu->opcode);
#endif
      modregrm(cpu);
      cpu->oper1b = readrm8(cpu, cpu->rm);
      op_grp3_8(cpu);
      loopcount +=4;
      if ((cpu->reg > 1) && (cpu->reg < 4)) {
        writerm8(cpu, cpu->rm, cpu->res8);
        loopcount +=6;
      }
      break;
    }
    case 0xF7: {    // F7 GRP3b Ev
#ifdef DEBUG_DISASM
      debug_log(DEBUG_DETAIL, "group3 16, opcode: %02X\t", cpu->opcode);
#endif
      modregrm(cpu);
      cpu->oper1 = readrm16(cpu, cpu->rm);
      op_grp3_16(cpu);
      loopcount +=4;
      if ((cpu->reg > 1) && (cpu->reg < 4)) {
        writerm16(cpu, cpu->rm, cpu->res16);
        loopcount +=6;
      }
      break;
    }
    case 0xFE: {    // FE GRP4 Eb
#ifdef DEBUG_DISASM
      debug_log(DEBUG_DETAIL, "[DASM] [not implemented] %04X:%04X, opcode: %02X\t", cpu->segregs[regcs], cpu->ip, cpu->opcode);
#endif
      modregrm(cpu);
      cpu->oper1b = readrm8(cpu, cpu->rm);
      cpu->oper2b = 1;
      if (!cpu->reg) {
#ifdef DEBUG_DISASM
        debug_log(DEBUG_DETAIL, "inc\n", cpu->segregs[regcs], cpu->ip, cpu->opcode);
#endif
        cpu->tempcf = cpu->cf;
        cpu->res8 = cpu->oper1b + cpu->oper2b;
        flag_add8(cpu, cpu->oper1b, cpu->oper2b);
        cpu->cf = cpu->tempcf;
        writerm8(cpu, cpu->rm, cpu->res8);
      } else {
#ifdef DEBUG_DISASM
        debug_log(DEBUG_DETAIL, "dec\n", cpu->segregs[regcs], cpu->ip, cpu->opcode);
#endif
        cpu->tempcf = cpu->cf;
        cpu->res8 = cpu->oper1b - cpu->oper2b;
        flag_sub8(cpu, cpu->oper1b, cpu->oper2b);
        cpu->cf = cpu->tempcf;
        writerm8(cpu, cpu->rm, cpu->res8);
      }
      break;
    }
    case 0xFF: {    // FF GRP5 Ev
#ifdef DEBUG_DISASM
      debug_log(DEBUG_DETAIL, "[DASM] [not implemented] %04X:%04X, opcode: %02X\t", cpu->segregs[regcs], cpu->ip, cpu->opcode);
#endif
      modregrm(cpu);
      cpu->oper1 = readrm16(cpu, cpu->rm);
      op_grp5(cpu);
      break;
    }

    // Invalid opcodes on 8086
    case 0x60 ... 0x6F:
    case 0xC0:
    case 0xC1:
    case 0xC8:
    case 0xC9:
    case 0xD6: {    // D6 SALC      // illegal on 8086
      debug_log(DEBUG_DETAIL, "[DASM] [new] %04X:%04X, op: %02X\t\tIllegal instruction\n", cpu->savecs, cpu->saveip, cpu->opcode);
      break;
    }
    default:   {
      debug_log(DEBUG_DETAIL, "[DASM] [new] %04X:%04X, op: %02X\t\tIllegal instruction [default case]\n", cpu->savecs, cpu->saveip, cpu->opcode);
      //cpu_intcall(cpu, 6); // trip invalid opcode exception.
      // this occurs on the 80186+, 8086/8088 CPUs treat them as NOPs.
      // technically they aren't exactly like NOPs in most cases,
      // but for our pursoses, that's accurate enough.
      break;
    }
  }

  return loopcount;

}

void do8087(CPU_t* cpu) {
  //modregrm(cpu);
#ifdef DEBUG_FPU
  uint8_t fAddrTmp;
  fAddrTmp = getmem8(cpu, cpu->segregs[regcs], cpu->ip);
  debug_log(DEBUG_INFO, "[8087] exec: Addr: %04X:%04X, opcode: %02X\r\n", cpu->segregs[regcs], cpu->ip, cpu->opcode);
  debug_log(DEBUG_INFO, "[8087] operand 2nd byte: 0x%02X\r\n", fAddrTmp);
#endif
  StepIP(cpu, 1);
#ifdef DEBUG_FPU
  debug_log(DEBUG_INFO, "[8087] regs: AX: %04X, BX: %04X, CX: %04X, DX: %04X\r\n", cpu->regs.wordregs[regax], cpu->regs.wordregs[regbx], cpu->regs.wordregs[regcx], cpu->regs.wordregs[regdx]);
  debug_log(DEBUG_INFO, "[8087] regs: SI: %04X, DI: %04X, BP: %04X, SP: %04X\r\n", cpu->regs.wordregs[regsi], cpu->regs.wordregs[regdi], cpu->regs.wordregs[regbp], cpu->regs.wordregs[regsp]);
  debug_log(DEBUG_INFO, "[8087] regs: CS: %04X, DS: %04X, ES: %04X, SS: %04X\r\n", cpu->segregs[regcs], cpu->segregs[regds], cpu->segregs[reges], cpu->segregs[regss]);
#endif
}
