// cpCatchAll commands
#define catchReadDelayRepeat    0
#define catchWriteDelayRepeat   1
#define catchReadStatusKey      2
#define catchSysControl         3
#define catchRepeat             4
#define catchKeyboard           5
#define catchWatchdog           6
#define catchBlank              7
#define catchInvert             8
#define catchDma                9
#define catchResetWatchDog      10
#define catchSetWatchDog        11
#define catchReadKbdStatus      12

// keyboard commands
#define keyboardCommand01     0x01  // unknown catchKeyboard
#define keyboardCommand02     0x02  // unknown catchWatchdog
#define keyboardCommand08     0x08  // unknown catchRepeat

#define keyboardCommand18     0x18  // unknown, saw in BIOS
#define keyboardCommand19     0x19  // unknown, saw in BIOS and in CCOS

#define keyboardCommand10     0x10  // unknown catchDma

#define keyboardCommand81     0x81  // repeat rate high byte
#define keyboardCommand82     0x82  // repeat rate low byte

// for direct write to command port
#define keyboardCommand83     0x83  // set watchdog
#define keyboardCommandC0     0xC0  // reset watchdog

// unknown device (PAL near UART?)
#define hwUnknownSeg    0xDFE0
#define hwUnknownAddr6  0x06

// fake function for I/O
void portWrite(uint16_t segment, uint16_t offset, uint8_t data);
uint8_t portRead(uint16_t segment, uint16_t offset);

// global APIs
void keyboardSendCommand(uint8_t command);      // there is case with ah register
void keyboardWriteDataPort(uint8_t data);       // wait for keyboard ready, then write to data port
void keyboardWriteCommandPort(uint8_t data);    // wait for keyboard ready, then write to command port

// global variables in BIOS RAM Area
uint16_t keyboardDelayRepeat;
uint16_t keyboardStatusKey;


uint16_t cpCatchAll1101(uint8_t command, uint16_t data) {
    uint16_t ret;
    
    switch (command) {
        case catchReadDelayRepeat:
            ret = keyboardDelayRepeat;
            break;
        case catchWriteDelayRepeat:
            keyboardSendCommand(keyboardCommand81);
            keyboardWriteDataPort( (uint8_t)(data & 0xff00) >> 8 );
            keyboardSendCommand(keyboardCommand82);
            keyboardWriteDataPort( (uint8_t)(data & 0xff) );
            keyboardDelayRepeat = data;
            break;
        case catchReadStatusKey:
            ret = keyboardStatusKey;
            break;
        case catchSysControl:
            portWrite(hwUnknownSeg, hwUnknownAddr6, command & 0x01);
            break;
        case catchRepeat:
            keyboardSendCommand(keyboardCommand08);
            break;
        case catchKeyboard:
            keyboardSendCommand(keyboardCommand01);
            break;
        case catchWatchdog:
            keyboardSendCommand(keyboardCommand02);
            break;
        case catchBlank:
            break;
        case catchInvert:
            break;
        case catchDma:
            keyboardSendCommand(keyboardCommand10);
            break;
        case catchResetWatchDog:
            keyboardWriteCommandPort(keyboardCommandC0);
            break;
        case catchSetWatchDog:
            keyboardWriteCommandPort(keyboardCommand83);
            keyboardWriteDataPort(data);
            break;
        case catchReadKbdStatus:
            ret = keyboardCommandLow;  // mov al, ds:0xC; ds= BIOS data segment
            break;
    }
    return data;
}
