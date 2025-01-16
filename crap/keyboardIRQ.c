#define keyboardIRQnum 2
#define keyboardHWaddress 0xDFFC0
#define dataPort 0
#define commandPort 1

void endIRQ(uint8_t irqNum);
void doKeyboardHandler();
void doWatchDogHandler();
void keyboardSendCommand(uint8_t ah, uint8_t al);
uint8_t readKeyboardPort(uint8_t portNum);

uint16_t keyboardStatusKey;     // somewhere in global BIOS RAM area

uint8_t readKeyboardPort(uint8_t portNum) {
    // do imaginary magic with 8086 real mode asm 
    // "in al, (keyboardHWaddress + portNum << 1)"
}

void keyboardIRQ() {
    uint8_t al, ah;
    endIRQ( keyboardIRQnum );
    al = readKeyboardPort( dataPort );
    ah = readKeyboardPort( commandPort );
    
    if ( al == 0xFF ) {
        return;
    } else if ( al == 0xFE ) {
        return;
    } else if ( al == 0xFD ) {
        keyboardSendCommand( 0x1, 0x10 ) //al = 0x10; ah = 0x01;
        doWatchDogHandler();
    } else {
        keyboardStatusKey = ( ah << 8 ) | al ;
        doKeyboardHandler();
    }
}
