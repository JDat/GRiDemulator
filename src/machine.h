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

#ifndef _MACHINE_H_
#define _MACHINE_H_

#include "config.h"
#include <stdint.h>
#include "cpu.h"
#include "i8259.h"
//#include "i8253.h"
#include "input.h"

#define MACHINE_MEM_RAM			0
#define MACHINE_MEM_ROM			1
#define MACHINE_MEM_ENDLIST		2

#define MACHINE_ROM_OPTIONAL	0
#define MACHINE_ROM_REQUIRED	1
#define MACHINE_ROM_ISNOTROM	2

//#define MACHINE_HW_RTC					0x0000000000000100ULL

//the "skip" HW flags are set in args.c to make sure machine init functions don't override explicit settings from the command line
//#define MACHINE_HW_SKIP_RTC				0x0400000000000000ULL

typedef struct {
	CPU_t CPU;

	KEYSTATE_t KeyState;
	//uint64_t hwflags;
} MACHINE_t;

typedef struct {
	uint8_t memtype;
	uint32_t start;
	uint32_t size;
	uint8_t required;
	char* filename;
} MACHINEMEM_t;

typedef struct {
	char* id;
	char* description;
	int (*init)(MACHINE_t* machine);
//	uint8_t video;
	double speed;
	//uint64_t hwflags;
} MACHINEDEF_t;

//MACHINE_t machine;

//int machine_init_generic_xt(MACHINE_t* machine);
int machine_init_grid(MACHINE_t* machine);
int machine_init(MACHINE_t* machine, char* id);
void machine_list();

#endif
