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
#define addressLen  0x10

uint8_t rtc_read(void* dummy, uint32_t addr) {
        addr = addr - baseAddress;
#ifdef DEBUG_RTC
	debug_log(DEBUG_DETAIL, "[RTC] Read port 0x%02X\n", addr);
#endif
        uint8_t ret = 0xFF;
	
        struct tm *tdata;
        time_t rawtime;

        time(&rawtime);
        tdata = localtime(&rawtime);
        

	addr &= 0x1F;
	switch (addr) {
        case 0:                 // test register (not implemented)
                ret = 0;
                break;
	case 1:                 // 0.1 seconds (not implemented)
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
                ret = (uint8_t)tdata->tm_min;
                ret = ret / 10;
		break;
	case 6:                 // ones of hours
                ret = (uint8_t)tdata->tm_hour;
		ret = ret  % 10;
                break;
	case 7:                 // tenths of hour
                ret = (uint8_t)tdata->tm_hour;
                ret = ret / 10;
		break;
        case 8:                 // ones of days
                ret = (uint8_t)tdata->tm_mday;
                ret = ret  % 10;
	case 9:                 // tenths of days
                ret = (uint8_t)tdata->tm_mday;
                ret = ret / 10;
		break;
        case 10:                // weekday
                ret = (uint8_t)tdata->tm_wday;
                break;
        case 11:                // ones of months
                ret = (uint8_t)tdata->tm_mon;
                ret = ret  % 10;
                break;
        case 12:                // tenths of months
                ret = (uint8_t)tdata->tm_mon;
                ret = ret / 10;
                break;
        case 13:                // Year, write only
                ret = 0;
                break;
        case 14:                // Start/stop clock, write only
                ret = 0;
                break;
        case 15:                // Interrupt setup, not implemented
                ret = 0;
                break;
	}
	return ret;
}

//void rtc_write(void* dummy, uint16_t addr, uint8_t value) {
void rtc_write(void* dummy, uint32_t addr, uint8_t value) {
        addr = addr - baseAddress;
#ifdef DEBUG_RTC
        debug_log(DEBUG_DETAIL, "[RTC] Write port 0x%02X: %02X\n", addr, value);
#endif
        // time set from GRiD sofware not implemented yet
}

void rtc_init() {
#ifdef DEBUG_RTC
	debug_log(DEBUG_INFO, "[RTC] Initializing real time clock\r\n");
#endif
        memory_mapCallbackRegister(baseAddress, addressLen, (void*)rtc_read, (void*)rtc_write, NULL);
}
