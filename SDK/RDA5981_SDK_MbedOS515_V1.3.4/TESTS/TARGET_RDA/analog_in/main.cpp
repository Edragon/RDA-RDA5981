#include "mbed.h"
#include "rtos.h"

/* Supported pins: ADC_PIN0, ADC_PIN1(U02)/ADC_PIN1A(U04), ADC_PIN2(VBAT) */
AnalogIn ain(ADC_PIN0);

int main(void)
{
    printf("Start AnalogIn test...\r\n");

    while (true) {
        float fval = ain.read();
        unsigned short ival = ain.read_u16();
        /* Print the percentage and 16-bit normalized values */
        printf("percentage: %3.3f%%\r\n", fval*100.0f);
        printf("normalized: 0x%04X\r\n\r\n", ival);
        Thread::wait(1000);
    }
}

