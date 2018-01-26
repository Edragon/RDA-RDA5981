#include "mbed.h"

/* SPI Pins: mosi, miso, sclk, ssel */
SPI spi(PC_0, PC_1, PB_4, PB_5);

int main()
{
    int rdata, wdata = 0x00;

    printf("Start SPI test...\r\n");

    /* Setup the spi for 8 bit data, high state clock, */
    /* second edge capture, with a 1MHz clock rate     */
    spi.format(8, 3);
    spi.frequency(1000000);

    while(true) {
        /* Send 1 byte */
        spi.write(wdata);
        wdata++;
        wdata &= 0x00FF;
        printf("Send data: 0x%X\r\n", wdata);

        /* Send 1 dummy byte to receive data from slave SPI */
        rdata = spi.write(0xFF);
        printf("Recv data: 0x%X\r\n", rdata);
    }
}
