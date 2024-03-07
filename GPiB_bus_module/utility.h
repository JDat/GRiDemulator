#ifndef _UTILITY_H_
#define _UTILITY_H_

#include <stdint.h>

#define bitRead(value, bit) (((value) >> (bit)) & 0x01)
#define bitSet(value, bit) ((value) |= (1UL << (bit)))
#define bitClear(value, bit) ((value) &= ~(1UL << (bit)))
#define bitToggle(value, bit) ((value) ^= (1UL << (bit)))
#define bitWrite(value, bit, bitvalue) ((bitvalue) ? bitSet(value, bit) : bitClear(value, bit))


int utility_loadFile(uint8_t* dst, size_t len, char* srcfile);
void utility_sleep(uint32_t ms);

#endif
