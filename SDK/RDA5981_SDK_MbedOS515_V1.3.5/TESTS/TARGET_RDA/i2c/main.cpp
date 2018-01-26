#include "mbed.h"

/* I2C Pins: sda, scl */
I2C i2c(PB_2, PB_3);
//I2C i2c(PD_0, PD_1);

int main()
{
    const int addr = 0x14 << 1;
    char txbuf[4] = {0x22, 0x44, 0x42, 0x44};
    char rxbuf[4] = {0x00};
    int idx = 0;

    printf("Start I2C test...\r\n");

    /*i2c.write(addr, txbuf, 1);
    i2c.read(addr, rxbuf, 1);
    printf("SlaveAddr: 0x%02X\r\n", addr);
    printf("Write: %02X, Read: %02X\r\n", txbuf[0], rxbuf[0]);

    i2c.write(addr, txbuf, 4);
    i2c.read(addr, rxbuf, 4);
    printf("Write:");
    for(idx = 0; idx < 4; idx++) {
        printf(" %02X,", txbuf[idx]);
    }
    printf("\r\nRead:");
    for(idx = 0; idx < 4; idx++) {
        printf(" %02X,", rxbuf[idx]);
    }
    printf("\r\n");*/

    txbuf[0] = 0xCD; // reg addr
    txbuf[1] = 0x0F; // byte 1
    txbuf[2] = 0x07; // byte 0
    i2c.write(addr, txbuf, 3);
    printf("SlaveAddr: 0x%02X, reg: 0x%02X, val:0x%02X%02X\r\n",
           addr, txbuf[0], txbuf[1], txbuf[2]);

    while(true) {
    }
}
