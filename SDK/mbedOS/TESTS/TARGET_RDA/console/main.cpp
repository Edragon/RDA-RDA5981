#include "mbed.h"
#include "rtos.h"
#include "console.h"

#define UART_BUF_THRESHOLD      (64)
#define CUSTOM_TIME             (1256729737)

int test_cmd_func(cmd_tbl_t *cmd, int argc, char *argv[])
{
    unsigned int arg1, arg2;

    if ((argc < 3)) {
        console_cmd_usage(cmd);
        return -1;
    }
    printf("Test Start\r\n");
    arg1 = console_strtoul(argv[1], NULL, 16);
    arg2 = console_strtoul(argv[2], NULL, 0);
    printf("arg1=0x%08X, arg2=%d\r\n", arg1, arg2);
    printf("Test Done\r\n");

    return 0;
}

int uart_echo_func(cmd_tbl_t *cmd, int argc, char *argv[])
{
    unsigned int total_len, len;
    time_t last_sec, curr_sec;
    unsigned char local_buf[UART_BUF_THRESHOLD] = {0};
    Timer tmr;

    if ((argc < 2)) {
        console_cmd_usage(cmd);
        return -1;
    }

    //set_time(CUSTOM_TIME);  // Set RTC time to Wed, 28 Oct 2009 11:35:37
    tmr.start();

    total_len = console_strtoul(argv[1], NULL, 0);
    printf("Uart Echo Start, total length: %d\r\n", total_len);

    //last_sec = curr_sec = time(NULL);
    last_sec = curr_sec = tmr.read_ms() / 1000;
    len = 0;
    while(len < total_len) {
        //curr_sec = time(NULL);
        curr_sec = tmr.read_ms() / 1000;
        if((console_fifo_len() >= UART_BUF_THRESHOLD) || (curr_sec >= (last_sec + 1))) {
            unsigned int idx;
            unsigned int tmp_len = console_fifo_get(local_buf, UART_BUF_THRESHOLD);
            last_sec = curr_sec;
            if(tmp_len > 0) {
                console_puts("Echo > ");
                for(idx = 0; idx < tmp_len; idx++) {
                    console_putc(local_buf[idx]);
                }
                console_puts("\r\n");
                len += tmp_len;
            }
        }
        Thread::wait(100);
        //printf("%d,%d\r\n", last_sec, curr_sec);
    }

    printf("Uart Echo Done\r\n");

    return 0;
}

int main()
{
    cmd_tbl_t test_cmd = {
        "test", 3, test_cmd_func,
        "test arg1 arg2 - test cmd with 2 args\r\n"
    };

    cmd_tbl_t echo_cmd = {
        "echo", 3, uart_echo_func,
        "echo len - echo uart data with len\r\n"
    };

    printf("Start Console Test...\r\n");

    console_init();
    if(0 != console_cmd_add(&test_cmd)) {
        printf("Add cmd failed\r\n");
    }

    if(0 != console_cmd_add(&echo_cmd)) {
        printf("Add echo cmd failed\r\n");
    }

    while (true) {
    }
}
