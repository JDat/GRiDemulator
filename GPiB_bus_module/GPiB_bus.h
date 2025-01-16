#ifndef _GPiB_BUS_H_
#define _GPiB_BUS_H_

/*
 * Logic levels are the same as on real GPiB bus
 * High level on bus (+5V) is logic 1 (true) in code
 * Low level on bus (0V) is logic 0 (false) in code
 */
#include <stdint.h>
#include <stdbool.h>

#define DEVICE_COUNT 16

typedef void (*voidFuncPtr)(void);
typedef int8_t typeClientId;

enum {
    pinSRQ  = 0,        // slave
    pinATN  = 1,        // controller
    pinEOI  = 2,        // controller/slave
    pinDAV  = 3,        // controller
    pinNRFD = 4,        // slave
    pinNDAC = 5,        // slave
    pinIFC  = 6,        // controller
    pinREN  = 7         // controller
};

struct _client {
    bool            used;
    typeClientId    clientId;
    voidFuncPtr     receiverFunctionCallBack;
    uint8_t         data;
    uint8_t         status;
    char            description[32];
};

/*
 * Init variables and states, forget all previous data
 */
void GPiBinit();

/*
 * Register bus member
 * param1: provide pointer to callback function
 * param2: provide description of member
 * return -1 if error
 * return clientId of OK
 */
//int8_t GPiBregisterClient(void (*userFunc)(void), char desc[32]);
int8_t GPiBregisterClient(void (*userFunc)(void), char desc[]);

/*
 * For testing
 */
int8_t GPiBdumpClientsArray();

void GPiBbusSRQwrite(bool pin, typeClientId clientId);
bool GPiBbusSRQget();

void GPiBbusATNwrite(bool pin, typeClientId clientId);
bool GPiBbusATNget();

void GPiBbusEOIwrite(bool pin, typeClientId clientId);
bool GPiBbusEOIget();

void GPiBbusDAVwrite(bool pin, typeClientId clientId);
bool GPiBbusDAVget();

void GPiBbusNRFDwrite(bool pin, typeClientId clientId);
bool GPiBbusNRFDget();

void GPiBbusNDACwrite(bool pin, typeClientId clientId);
bool GPiBbusNDACget();

void GPiBbusIFCwrite(bool pin, typeClientId clientId);
bool GPiBbusIFCget();

void GPiBbusRENwrite(bool pin, typeClientId clientId);
bool GPiBbusRENget();

void GPiBbusStatusWrite(uint8_t status, typeClientId clientId);
uint8_t GPiBbusStatusGet();

void GPiBbusDataWrite(uint8_t data, typeClientId clientId);
uint8_t GPiBbusDataGet();

#endif
