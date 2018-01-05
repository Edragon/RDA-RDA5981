#include "mbed.h"
#include "USBMSD_SD.h"

int main() {
    printf("start USBMSD_SD test...\r\n");
    /* SDMMC Pins: clk, cmd, d0, d1, d2, d3 */
    USBMSD_SD sd(PB_9, PB_0, PB_3, PB_7, PC_0, PC_1);
    printf("USBMSD_SD test end\r\n");
    while(1);
    //Thread::wait(osWaitForever);
}
