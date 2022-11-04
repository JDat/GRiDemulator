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
	Machine definitions.
*/

#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <stdint.h>
#include "config.h"
#include "debuglog.h"
#include "cpu.h"
#include "i8259.h"
#include "i8253.h"
//#include "chipset/i8255.h"
//#include "chipset/uart.h"
//#include "modules/disk/biosdisk.h"
//#include "modules/disk/fdc.h"
//#include "modules/input/mouse.h"
#include "input.h"
#include "gridKeyboard8741.h"
#ifdef USE_NE2000
//#include "modules/io/ne2000.h"
//#include "modules/io/pcap-win32.h"
#endif
//#include "modules/io/tcpmodem.h"
#include "gridvideo.h"
#include "rtc.h"
#include "i8274.h"
#include "modem.h"
#include "i7220.h"
#include "tms9914a.h"
#include "memory.h"
#include "utility.h"
#include "timing.h"
#include "machine.h"

/*
	ID string, full description, init function, default video, speed in MHz (-1 = unlimited), default hardware flags
*/
const MACHINEDEF_t machine_defs[] = {
	{ "1101", "GRiD Compass 1101", machine_init_grid, VIDEO_CARD_GRID1101, 1, MACHINE_HW_RTC },
        //{ "1139", "GRiD Compass 1139", machine_init_grid, VIDEO_CARD_GRID1139, 5, MACHINE_HW_RTC },
        { NULL }
};

const MACHINEMEM_t machine_mem[][11] = {
        /*
	//Generic XT clone
	{
		{ MACHINE_MEM_RAM, 0x00000, 0xA0000, MACHINE_ROM_ISNOTROM, NULL },
#ifndef USE_DISK_HLE
		{ MACHINE_MEM_ROM, 0xD0000, 0x02000, MACHINE_ROM_REQUIRED, "roms/disk/ide_xt.bin" },
#endif
		{ MACHINE_MEM_ROM, 0xFE000, 0x02000, MACHINE_ROM_REQUIRED, "roms/machine/generic_xt/pcxtbios.bin" },
		{ MACHINE_MEM_ENDLIST, 0, 0, 0, NULL }
	},

	//IBM XT
	{
		{ MACHINE_MEM_RAM, 0x00000, 0xA0000, MACHINE_ROM_ISNOTROM, NULL },
#ifndef USE_DISK_HLE
		{ MACHINE_MEM_ROM, 0xD0000, 0x02000, MACHINE_ROM_REQUIRED, "roms/disk/ide_xt.bin" },
#endif
		{ MACHINE_MEM_ROM, 0xF0000, 0x08000, MACHINE_ROM_REQUIRED, "roms/machine/ibm_xt/5000027.u19" },
		{ MACHINE_MEM_ROM, 0xF8000, 0x08000, MACHINE_ROM_REQUIRED, "roms/machine/ibm_xt/1501512.u18" },
		{ MACHINE_MEM_ENDLIST, 0, 0, 0, NULL }
	},

	//AMI XT clone
	{
		{ MACHINE_MEM_RAM, 0x00000, 0xA0000, MACHINE_ROM_ISNOTROM, NULL },
#ifndef USE_DISK_HLE
		{ MACHINE_MEM_ROM, 0xD0000, 0x02000, MACHINE_ROM_REQUIRED, "roms/disk/ide_xt.bin" },
#endif
		{ MACHINE_MEM_ROM, 0xFE000, 0x02000, MACHINE_ROM_REQUIRED, "roms/machine/ami_xt/ami_8088_bios_31jan89.bin" },
		{ MACHINE_MEM_ENDLIST, 0, 0, 0, NULL }
	},

	//Phoenix XT clone
	{
		{ MACHINE_MEM_RAM, 0x00000, 0xA0000, MACHINE_ROM_ISNOTROM, NULL },
#ifndef USE_DISK_HLE
		{ MACHINE_MEM_ROM, 0xD0000, 0x02000, MACHINE_ROM_REQUIRED, "roms/disk/ide_xt.bin" },
#endif
		{ MACHINE_MEM_ROM, 0xFE000, 0x02000, MACHINE_ROM_REQUIRED, "roms/machine/phoenix_xt/000p001.bin" },
		{ MACHINE_MEM_ENDLIST, 0, 0, 0, NULL }
	},

	//Xi 8088
	{
		{ MACHINE_MEM_RAM, 0x00000, 0xA0000, MACHINE_ROM_ISNOTROM, NULL },
#ifndef USE_DISK_HLE
		{ MACHINE_MEM_ROM, 0xD0000, 0x02000, MACHINE_ROM_REQUIRED, "roms/disk/ide_xt.bin" },
#endif
		{ MACHINE_MEM_ROM, 0xF0000, 0x10000, MACHINE_ROM_REQUIRED, "roms/machine/xi8088/bios128k-2.0.bin" }, //last half of this ROM is just filler for the 128k chip...
		{ MACHINE_MEM_ENDLIST, 0, 0, 0, NULL }
	},

	//Zenith SuperSport 8088
	{
		{ MACHINE_MEM_RAM, 0x00000, 0xA0000, MACHINE_ROM_ISNOTROM, NULL },
#ifndef USE_DISK_HLE
		{ MACHINE_MEM_ROM, 0xD0000, 0x02000, MACHINE_ROM_REQUIRED, "roms/disk/ide_xt.bin" },
#endif
		{ MACHINE_MEM_RAM, 0xF0000, 0x04000, MACHINE_ROM_ISNOTROM, NULL }, //scratchpad RAM
		{ MACHINE_MEM_ROM, 0xF8000, 0x08000, MACHINE_ROM_REQUIRED, "roms/machine/zenithss/z184m v3.1d.10d" },
		{ MACHINE_MEM_ENDLIST, 0, 0, 0, NULL }
	},

	//Supersoft/Landmark diagnostic
	{
		{ MACHINE_MEM_RAM, 0x00000, 0xA0000, MACHINE_ROM_ISNOTROM, NULL },
		{ MACHINE_MEM_ROM, 0xF8000, 0x08000, MACHINE_ROM_REQUIRED, "roms/machine/landmark/landmark.bin" },
		{ MACHINE_MEM_ENDLIST, 0, 0, 0, NULL }
	},
*/
	// GRiD compass
        // GRid Compass 1101
	{
                { MACHINE_MEM_RAM, 0x00000, 0x00400, MACHINE_ROM_ISNOTROM, NULL },      // Interrupt table
                //{ MACHINE_MEM_RAM, 0x02980, 0x3D680, MACHINE_ROM_ISNOTROM, NULL },    // RAM after videobuffer 256 kb
                { MACHINE_MEM_RAM, 0x02980, 0x7D680, MACHINE_ROM_ISNOTROM, NULL },      // RAM after videobuffer 512 kb
                { MACHINE_MEM_RAM, 0xC0000, 0x0FFFF, MACHINE_ROM_ISNOTROM, NULL },      // Factory test ROM
                { MACHINE_MEM_ROM, 0xFC000, 0x04000, MACHINE_ROM_REQUIRED, "ROMS/bootROM1101.bin" },
		{ MACHINE_MEM_ENDLIST, 0, 0, 0, NULL }
	},
        // GRid Compass 1139
	//{
        //        { MACHINE_MEM_RAM, 0x00000, 0x00400, MACHINE_ROM_ISNOTROM, NULL },      // Interrupt table
        //        { MACHINE_MEM_RAM, 0x04400, 0x7BC00, MACHINE_ROM_ISNOTROM, NULL },      // RAM after videobuffer
        //        { MACHINE_MEM_RAM, 0xC0000, 0x0FFFF, MACHINE_ROM_ISNOTROM, NULL },
        //        { MACHINE_MEM_ROM, 0xFC000, 0x04000, MACHINE_ROM_REQUIRED, "ROMS/bootROM1139.bin" },
	//	{ MACHINE_MEM_ENDLIST, 0, 0, 0, NULL }
	//},
};

//uint8_t mac[6] = { 0xac, 0xde, 0x48, 0x88, 0xbb, 0xab };

int machine_init_grid(MACHINE_t* machine) {
	if (machine == NULL) return -1;

	//debug_log(DEBUG_INFO, "[MACHINE] Init Grid function start\r\n");
	i8259_init(&machine->i8259);
        i8253_init(&machine->i8253, &machine->i8259);
        
        gridKeyboard8741_init();
        //dmaInit();
        if (bubble_init(&machine->i8259)) {
                return -1;
        }
        uart_init();
        modem_init();
        
        tms9914a_init();
	//i8255_init(&machine->i8255, &machine->KeyState, &machine->pcspeaker);
        
        rtc_init(&machine->CPU);

	cpu_reset(&machine->CPU);

	if (gridvideo_init()) return -1;

	//debug_log(DEBUG_INFO, "[MACHINE] Init Grid function end\r\n");
	return 0;
}

int machine_init(MACHINE_t* machine, char* id) {
	int num = 0, match = 0, i = 0;

	do {
		if (machine_defs[num].id == NULL) {
			debug_log(DEBUG_ERROR, "[MACHINE] ERROR: Machine definition not found: %s\r\n", id);
			return -1;
		}

		//if (_stricmp(id, machine_defs[num].id) == 0) {
                if (strcasecmp(id, machine_defs[num].id) == 0) {
			match = 1;
		}
		else {
			num++;
		}
	} while (!match);

	debug_log(DEBUG_INFO, "[MACHINE] Initializing machine: \"%s\" (%s)\r\n", machine_defs[num].description, machine_defs[num].id);

	//Initialize machine memory map
	while(1) {
		uint8_t* temp;
		if (machine_mem[num][i].memtype == MACHINE_MEM_ENDLIST) {
			break;
		}
		temp = (uint8_t*)malloc((size_t)machine_mem[num][i].size);
		if ((temp == NULL) &&
			((machine_mem[num][i].required == MACHINE_ROM_REQUIRED) || (machine_mem[num][i].required == MACHINE_ROM_ISNOTROM))) {
			debug_log(DEBUG_ERROR, "[MACHINE] ERROR: Unable to allocate %lu bytes of memory\r\n", machine_mem[num][i].size);
			return -1;
		}
		if (machine_mem[num][i].memtype == MACHINE_MEM_RAM) {
			memory_mapRegister(machine_mem[num][i].start, machine_mem[num][i].size, temp, temp);
		} else if (machine_mem[num][i].memtype == MACHINE_MEM_ROM) {
			int ret;
                        debug_log(DEBUG_ERROR, "[MACHINE] Loading bootROM: %s\r\n", machine_mem[num][i].filename);
			ret = utility_loadFile(temp, machine_mem[num][i].size, machine_mem[num][i].filename);
			if ((machine_mem[num][i].required == MACHINE_ROM_REQUIRED) && ret) {
				debug_log(DEBUG_ERROR, "[MACHINE] Could not open file, or size is less than expected: %s\r\n", machine_mem[num][i].filename);
				return -1;
			}
			memory_mapRegister(machine_mem[num][i].start, machine_mem[num][i].size, temp, NULL);
		}
		i++;
	}

	machine->hwflags |= machine_defs[num].hwflags;

	if (videocard == 0xFF) {
		videocard = machine_defs[num].video;
	}

	if (speedarg > 0) {
		speed = speedarg;
	} else if (speedarg < 0) {
		speed = -1;
	} else {
		speed = machine_defs[num].speed;
	}

	if ((*machine_defs[num].init)(machine)) { //call machine-specific init routine
		return -1;
	}

	return num;
}

void machine_list() {
	int machine = 0;

	printf("Valid " STR_TITLE " machines:\r\n");

	while(machine_defs[machine].id != NULL) {
		printf("%s: \"%s\"\r\n", machine_defs[machine].id, machine_defs[machine].description);
		machine++;
	}
}
