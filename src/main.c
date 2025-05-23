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
#include <string.h>
#include "config.h"
#include "args.h"
#include "timing.h"
#include "memory.h"
#include "ports.h"
#include "machine.h"
#include "utility.h"
#include "debuglog.h"
#include "cpu.h"
#include "i8259.h"
#include "i8741a.h"
#include "sdlconsole.h"

#include "gridvideo.h"

char* usemachine = "1101"; //default

char title[64]; //assuming 64 isn't safe if somebody starts messing with STR_TITLE and STR_VERSION

uint64_t ops = 0;
//uint32_t baudrate = 115200, ramsize = 640, instructionsperloop = 100, cpuLimitTimer;
//uint8_t videocard = 0xFF, showMIPS = 0;
volatile uint8_t goCPU = 1, limitCPU = 0;
volatile double speed = 0;

volatile uint8_t running = 1;

MACHINE_t machine;

//void optimer(void* dummy) {
//void optimer() {
//    ops /= 10000;
//    if (showMIPS) {
//        debug_log(DEBUG_INFO, "%llu.%llu MIPS          \r", ops / 10, ops % 10);
//    }
//    ops = 0;
//}

//void cputimer(void* dummy) {
void cputimer() {
    goCPU = 1;
}

void setspeed(double mhz) {
    if (mhz > 0) {
        speed = mhz;
        //instructionsperloop = (uint32_t)((speed * 1000000.0) / 140000.0);
        limitCPU = 1;
        //debug_log(DEBUG_INFO, "[MACHINE] Throttling speed to approximately a %.02f MHz 8086 (%lu instructions/sec)\r\n", speed, instructionsperloop * 10000);
        //timing_timerEnable(cpuLimitTimer);
    }
    else {
        speed = 0;
        //instructionsperloop = 100;
        limitCPU = 0;
        //timing_timerDisable(cpuLimitTimer);
    }
}

int main(int argc, char *argv[]) {

        uint32_t keyScanCode;
        uint8_t modKeys;

    sprintf(title, "%s v%s pre alpha", STR_TITLE, STR_VERSION);

        printf("%s\r\n", title);

    memory_init();
    ports_init();
    timing_init();

    if (args_parse(&machine, argc, argv)) {
        return -1;
    }

    if (sdlconsole_init(title)) {
        debug_log(DEBUG_ERROR, "[ERROR] SDL initialization failure\r\n");
        return -1;
    }

    if (machine_init(&machine, usemachine) < 0) {
        debug_log(DEBUG_ERROR, "[ERROR] Machine initialization failure\r\n");
        return -1;
    }

    //timing_addTimer(optimer, 10, TIMING_ENABLED);
    //cpuLimitTimer = timing_addTimer(cputimer, 10000, TIMING_DISABLED);
    //if (speed > 0) {
        //setspeed(speed);
    //}
    while (running) {
        timing_loop();

        static uint32_t curloop = 0;
        
        //limitCPU = 0;
        //if (limitCPU == 0) {
            goCPU = 1;
        //}
        //if (goCPU) {
            cpu_interruptCheck(&machine.CPU);
                        //doDMA();
            //cpu_exec(&machine.CPU, instructionsperloop);
            cpu_exec(&machine.CPU);
            //ops += instructionsperloop;
            //goCPU = 0;
        //}
        if (++curloop == 100) {
            gridvideo_update();

            switch (sdlconsole_loop()) {
                                case SDLCONSOLE_EVENT_KEY:
                                        keyScanCode = sdlconsole_getScanCode();
                                        modKeys = sdlconsole_getModKeys();
                                        gridKeyboard8741_getScanCode(keyScanCode, modKeys);
                                        break;
                                case SDLCONSOLE_EVENT_QUIT:
                                        running = 0;
                                        break;
                                case SDLCONSOLE_EVENT_DEBUG_1:
                                        ramDump(0,0x40000);
                                        break;
                                case SDLCONSOLE_EVENT_DEBUG_2:
                                        //if (limitCPU == 1) {
                                                //limitCPU = 0;
                                                //debug_log(DEBUG_INFO, "[MACHINE] Run\n");
                                        //} else {
                                                //limitCPU = 1;
                                                //debug_log(DEBUG_INFO, "[MACHINE] Stop\n");
                                        //}
                                        break;
            }
            curloop = 0;
        }
    }

    return 0;
}
