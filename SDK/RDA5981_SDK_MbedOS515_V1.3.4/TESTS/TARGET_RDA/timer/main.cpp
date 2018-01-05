#include "mbed.h"
#include "rtos.h"

Timer tmr;

int main()
{
    printf("Start timer test...\r\n");
    /* Note: The value of macro "OS_CLOCK" must be equal to varialble "SystemCoreClock" */
    printf("CPU_CLK: %d\r\n", SystemCoreClock);
    printf("BUS_CLK: %d\r\n", AHBBusClock);

    tmr.start();
    while (true) {
        printf("tmr=%d\r\n", tmr.read_ms());
        Thread::wait(500);
    }
}
