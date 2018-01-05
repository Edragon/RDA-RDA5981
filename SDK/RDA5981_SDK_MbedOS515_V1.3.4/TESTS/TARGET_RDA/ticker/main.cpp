#include "mbed.h"
#include "rtos.h"

Ticker tckr;
LowPowerTicker lp_tckr;
DigitalOut led0(GPIO_PIN24);
DigitalOut led1(GPIO_PIN25);

void cb_func(void)
{
    led0 = !led0;
    mbed_error_printf("ticker_cb\r\n");
}

void lp_cb_func(void)
{
    led1 = !led1;
    mbed_error_printf("lp_ticker_cb\r\n");
}

int main()
{
    printf("\r\nStart ticker test...\r\n");
    /* Note: The value of macro "OS_CLOCK" must be equal to varialble "SystemCoreClock" */
    printf("CPU_CLK: %d\r\n", SystemCoreClock);
    printf("BUS_CLK: %d\r\n", AHBBusClock);

    tckr.attach(&cb_func, 3); // 3s
    lp_tckr.attach(&lp_cb_func, 2); // 2s
    while (true) {
        printf("loop\r\n");
        Thread::wait(1000); // 1000ms
    }
}

