#include "mbed.h"
#include "rtos.h"

#define CUSTOM_TIME  1256729737

int main() {
    char buffer[32] = {0};
    set_time(CUSTOM_TIME);  // Set RTC time to Wed, 28 Oct 2009 11:35:37

    while (true) {
        time_t seconds;

        /* Delay 1s using systick timer */
        Thread::wait(1000);

        /* Get RTC timer */
        seconds = time(NULL);
        strftime(buffer, sizeof(buffer), "%Y-%m-%d %H:%M:%S %p", localtime(&seconds));
        printf("[%ld] %s\r\n", seconds, buffer);
    }
}

