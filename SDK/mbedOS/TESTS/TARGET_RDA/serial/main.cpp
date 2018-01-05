#include "mbed.h"

/* UART Serial Pins: tx, rx */
Serial serial(UART1_TX, UART1_RX);

int main()
{
    serial.baud(921600);
    serial.format(8, SerialBase::None, 1);

    serial.printf("Start Serial test...\r\n");

    while(true) {
        serial.putc(serial.getc());
    }
}
