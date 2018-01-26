#include "mbed.h"

/* UART Serial Pins: tx, rx */
Serial serial(UART1_TX, UART1_RX);

void rx_irq_handler(void)
{
    while(serial.readable()) {
        serial.putc(serial.getc());
    }
}

int main()
{
    serial.baud(921600);
    serial.format(8, SerialBase::None, 1);

    serial.printf("Start Serial test...\r\n");

    serial.attach(rx_irq_handler, SerialBase::RxIrq);

    serial.printf("Wait forever\r\n");
    Thread::wait(osWaitForever);
}

