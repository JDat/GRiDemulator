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


#include <stdio.h>
#include <stdint.h>
#include <stdbool.h>
#include <stdlib.h>
#include "config.h"
#include "cpu.h"
#include "utility.h"
#include "memory.h"
#include "debuglog.h"

#include "machine.h"

bool dmaActive;
uint16_t  dmaCount;

//void doDMA() {
        //debug_log(DEBUG_ERROR, "[DMA] doDMA\n");
        //while (dmaActive) {
                //dmaCount++;
                //if (dmaCount > 256) {
                        //dmaActive = false;
                //}
                //dmaRead(DMABASE);
        //}
//}

//void dmaBubbleRequest() {
//        dmaActive  = true;
//        dmaRead(DMABASE);
//}

FILE *dmaFile;

int dmaInit() {
/*
        if (dmaFile != NULL) {
                fclose(dmaFile);
        }
        dmaFile = fopen("ROMS/merged.raw", "rb");
        if (dmaFile == NULL) {

                debug_log(DEBUG_INFO, "[DMA] Error openimg DMA image\r\n");

        return -1;
        }
    
        memory_mapCallbackRegister(DMABASE, DMALEN, (void*)dmaRead, (void*)dmaWrite, NULL);
*/
        return 0;
}


uint8_t dmaRead(uint32_t addr) {
        //size_t ret;
        uint8_t val = 0;
        
        //val = bubble_read(NULL, 0xDFE80);
        //debug_log(DEBUG_ERROR, "[DMA] Read. Addr: %05X, Value: %02X\r\n", addr, val);
        //ret = fread(&val, 1, 1, dmaFile);

        //(void)fread(&val, 1, 1, dmaFile);
        return val;
        
}

void dmaWrite(uint32_t addr, uint8_t value) {
        //debug_log(DEBUG_ERROR, "[DMA] Write. Addr: %05X\tValue: %02X\r\n", addr, value);
        //bubble_write(NULL, 0xDFE80, value);
}
