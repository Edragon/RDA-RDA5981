#include "mbed.h"
#include "rtos.h"
#include "rda_wdt_api.h"
#include "console.h"

int main()
{
    int cnt = 0;

    printf("Start watchdog timer test...\r\n");
    printf("CPU_CLK: %d\r\n", SystemCoreClock);
    printf("BUS_CLK: %d\r\n", AHBBusClock);

    console_init();
    while(true) {
        printf("Alive loop\r\n");
        Thread::wait(1000);
        if(cnt == 8) {
            rda_wdt_softreset();
            while(true) {
                Thread::wait(1000);
                printf("Dead loop\r\n");
            }
        }
        cnt++;
    }
}

