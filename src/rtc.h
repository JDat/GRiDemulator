#ifndef _RTC_H_
#define _RTC_H_

#include <stdint.h>
//#include "cpu.h"

uint8_t rtc_read(void* dummy, uint32_t addr);
void rtc_write(void* dummy, uint32_t addr, uint8_t value);

//uint8_t rtc_read(void* dummy, uint16_t addr);
//void rtc_write(void* dummy, uint16_t addr, uint8_t value);
void rtc_init();

#endif