#include "mbed.h"
#include "rtos.h"
#include "rda_wdt_api.h"
#include "rda_ccfg_api.h"
#include "console.h"

int main()
{
    wdt_t obj;
    int cnt = 0;

    printf("Start watchdog timer test...\r\n");
    printf("CPU_CLK: %d\r\n", SystemCoreClock);
    printf("BUS_CLK: %d\r\n", AHBBusClock);
    printf("Abort Flag: %d\r\n", rda_ccfg_abort_flag());

    console_init();
    rda_wdt_init(&obj, 2);
    rda_wdt_start(&obj);
    while(true) {
        printf("Alive loop\r\n");
        Thread::wait(1500);
        if(cnt == 10) {
            while(true) {
                Thread::wait(1000);
                printf("Dead loop\r\n");
            }
        }
        cnt++;
        rda_wdt_feed(&obj);
    }
}

