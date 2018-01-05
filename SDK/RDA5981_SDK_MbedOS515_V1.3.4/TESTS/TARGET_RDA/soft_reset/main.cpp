#include "mbed.h"
#include "rtos.h"
#include "rda_wdt_api.h"

int main()
{
    int cnt = 0;

    printf("Start soft reset test...\r\n");
    printf("CPU_CLK: %d\r\n", (int)SystemCoreClock);

    while(true) {
        printf("loop\r\n");
        Thread::wait(500);
        if(cnt == 4) {
            /* software reset */
            rda_sys_softreset();
        }
        cnt++;
    }
}

