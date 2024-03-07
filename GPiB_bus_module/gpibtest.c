#include <stdio.h>
#include "GPiB_bus.h"

void masterCallBackFunc() {
    printf("Master CallBack executed\t");
    printf("Data: 0x%02X\t", GPiBbusDataGet());
    printf("Status: 0x%02X\n", GPiBbusStatusGet());
}

void fddCallBackFunc() {
    printf("fdd CallBack executed\t");
    printf("Data: 0x%02X\t", GPiBbusDataGet());
    printf("Status: 0x%02X\n", GPiBbusStatusGet());

}
void hddCallBackFunc() {
    printf("HDD CallBack executed\t");
    printf("Data: 0x%02X\t", GPiBbusDataGet());
    printf("Status: 0x%02X\n", GPiBbusStatusGet());

}


int main(int argc, char *argv[]) {
    //int8_t ret;
    GPiBinit();
    printf("id: %d\n", GPiBregisterClient(masterCallBackFunc, "Master"));
    printf("id: %d\n", GPiBregisterClient(fddCallBackFunc, "FDD"));
    printf("id: %d\n", GPiBregisterClient(hddCallBackFunc, "HDD"));
    //printf("Ret: %u\n", ret);
    GPiBdumpClientsArray();
    
    GPiBbusATNwrite(false, 0);
    GPiBbusSRQwrite(false, 1);
    GPiBbusDAVwrite(false, 2);
    printf("Data: 0x%02X\t", GPiBbusDataGet());
    printf("Status: 0x%02X\n", GPiBbusStatusGet());
    GPiBdumpClientsArray();
    return 0;
}
