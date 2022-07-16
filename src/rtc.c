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
	Generic RTC interface for XT systems, works with TIMER.COM version 1.2
*/

#include <stdio.h>
#include <stdint.h>
#include <string.h>
#include "config.h"
#include "rtc.h"
//#include "ports.h"
#include "memory.h"
#include "debuglog.h"
#include <time.h>

#define baseAddress 0xDFF40
#define addressLen  0x16

//uint8_t rtc_read(void* dummy, uint16_t addr) {
uint8_t rtc_read(void* dummy, uint32_t addr) {
        addr = addr - baseAddress;
	debug_log(DEBUG_DETAIL, "[RTC] Read port 0x%02X\n", addr);
        uint8_t ret = 0xFF;
	
        struct tm *tdata;
        time_t rawtime;

	//time(&tdata);
        time(&rawtime);
        tdata = localtime(&rawtime);
        
        //time(tdata);
        //tdata = time(NULL);

	addr &= 0x1F;
	switch (addr) {
        case 0:                 // test register
                ret = 0;
                break;
	case 1:                 // 0.1 seconds
		ret = 0;
		break;
	case 2:                 // ones of seconds
                ret = (uint8_t)tdata->tm_sec;
                ret = ret % 10;
		break;
	case 3:                 // tenths of seconds
                ret = (uint8_t)tdata->tm_sec;
                ret = ret / 10;
		break;
	case 4:                 // ones of minutes
                ret = (uint8_t)tdata->tm_min;
                ret = ret  % 10;
		break;
	case 5:                 // tenths of minutes
		//ret = (uint8_t)tdata.tm_wday;
                //ret = (uint8_t)tdata->tm_wday;
                ret = (uint8_t)tdata->tm_min;
                ret = ret / 10;
		break;
	case 6:                 // ones of hours
		//ret = (uint8_t)tdata.tm_mday;
                //ret = (uint8_t)tdata->tm_mday;
                ret = (uint8_t)tdata->tm_hour;
		ret = ret  % 10;
                break;
	case 7:                 // tenths of hour
		//ret = (uint8_t)tdata.tm_mon;
                //ret = (uint8_t)tdata->tm_mon;
                ret = (uint8_t)tdata->tm_hour;
                ret = ret / 10;
		break;
        case 8:
                ret = (uint8_t)tdata->tm_mday;
                ret = ret  % 10;
	case 9:
		//ret = (uint8_t)tdata.tm_year % 100;
                //ret = (uint8_t)tdata->tm_year % 100;
                ret = (uint8_t)tdata->tm_mday;
                ret = ret / 10;
		break;
        case 10:
                ret = (uint8_t)tdata->tm_wday;
                break;
        case 11:
                ret = (uint8_t)tdata->tm_mon;
                ret = ret  % 10;
                break;
        case 12:
                ret = (uint8_t)tdata->tm_mon;
                ret = ret / 10;
                break;
        case 13:
                ret = 0;
                break;
        case 14:
                ret = 0;
                break;
        case 15:
                ret = 0;
                break;
	}
/*
	if (ret != 0xFF) {
		uint8_t rh, rl;
		rh = (ret / 10) % 10;
		rl = ret % 10;
		ret = (rh << 4) | rl;
	}
*/
	return ret;
}

//void rtc_write(void* dummy, uint16_t addr, uint8_t value) {
void rtc_write(void* dummy, uint32_t addr, uint8_t value) {
        addr = addr - baseAddress;
        debug_log(DEBUG_DETAIL, "[RTC] Write port 0x%02X: %02X\n", addr, value);
}

void rtc_init() {
	debug_log(DEBUG_INFO, "[RTC] Initializing real time clock\r\n");
	//ports_cbRegister(0x240, 0x18, (void*)rtc_read, NULL, (void*)rtc_write, NULL, NULL);
        memory_mapCallbackRegister(baseAddress, addressLen, (void*)rtc_read, (void*)rtc_write, NULL);
}
