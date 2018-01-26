/*****************************************************************************
* @file     console.cpp
* @date     26.07.2016
* @note
*
*****************************************************************************/
#include "console.h"
#include "serial_api.h"
#include "at.h"
#include "rda_sleep_api.h"

static unsigned char uart_sleep_mode = 0;
static int uart_sleep_flag = 0;

extern int stdio_uart_inited;
extern serial_t stdio_uart;
extern uint32_t g_cnt_uart0;
rda_console_t p_rda_console;

extern char console_buffer[CMD_CBSIZE];
extern char lastcommand[CMD_CBSIZE];

void console_putc(char c) {
    serial_putc(&stdio_uart, c);
}

void console_puts(const char *s) {
    while(*s) {
        serial_putc(&stdio_uart, *s++);
    }
}

void console_set_baudrate(unsigned int baudrate)
{
    serial_baud(&stdio_uart,baudrate);
    serial_clear(&stdio_uart);
    stdio_uart.uart->IER |= 1 << 0;
}

void console_cmd_exec(void) {
    uint8_t  ch;
    int16_t len;

    while(kfifo_get(&ch, 1)) {
        len = handle_char(ch, 0);
        if (len >= 0) {
            strcpy(lastcommand, console_buffer);
            core_util_critical_section_enter();
            p_rda_console.cmd_cnt--;
            core_util_critical_section_exit();
            if (len > 0) {
                if(run_command(lastcommand) < 0) {
                    RDA_AT_PRINT("command fail\r\n");
                }
            }
            handle_char('0', "r");//'r' means reset
        }
    }
}

static void console_rxisr_Callback(void) {
    while(serial_readable(&stdio_uart))
    {
        unsigned char c = serial_getc(&stdio_uart);
        kfifo_put(&c, 1);
        if((c == '\r') || (c == '\n') || (c == 0x03)) {
            p_rda_console.cmd_cnt++;
            if(1 == uart_get_sleep_mode()) {
                uart_wakeup();
            }
        }
    }
    if(p_rda_console.cmd_cnt > 0)
        osSemaphoreRelease(p_rda_console.console_sema.sema_id);
}

static void console_irq_handler(uint32_t id, SerialIrq event) {
    if(RxIrq == event) {
        console_rxisr_Callback();
    }
}

static void console_task(const void *arg) {
    while (1) {
        osSemaphoreWait(p_rda_console.console_sema.sema_id, osWaitForever);
        console_cmd_exec();
    }
}

void uart_set_sleep_mode(unsigned char is_sleep)
{
    uart_sleep_mode = is_sleep;
}

unsigned char uart_get_sleep_mode(void)
{
    return uart_sleep_mode;
}

int is_uart_sleep_allowed(void)
{
  return uart_sleep_flag;
}

void uart_wakeup(void)
{
    if(1 == uart_sleep_flag) {
        uart_sleep_flag = 0;
    }
}

void uart_sleep(void)
{
    if(0 == uart_sleep_flag) {
        uart_sleep_flag = 1;
    }
}

void console_init(void) {
    sleep_entry_t uart_entry = {
        uart_set_sleep_mode,
        is_uart_sleep_allowed,
        NULL
    };

    sleep_entry_register(&uart_entry);

    p_rda_console.cmd_cnt = 0;

#ifdef CMSIS_OS_RTX
    memset(p_rda_console.console_sema.data, 0, sizeof(p_rda_console.console_sema.data));
    p_rda_console.console_sema.def.semaphore = p_rda_console.console_sema.data;
#endif
    p_rda_console.console_sema.sema_id = osSemaphoreCreate(&p_rda_console.console_sema.def, 0);

    if (!stdio_uart_inited) {
        serial_init(&stdio_uart, STDIO_UART_TX, STDIO_UART_RX);
#if MBED_CONF_CORE_STDIO_BAUD_RATE
        serial_baud(&stdio_uart, MBED_CONF_CORE_STDIO_BAUD_RATE);
#endif /* MBED_CONF_CORE_STDIO_BAUD_RATE */
    }

    serial_irq_handler(&stdio_uart, console_irq_handler, (uint32_t)(&p_rda_console));
    serial_irq_set(&stdio_uart, RxIrq, 1);

    init_console_irq_buffer();

    static osThreadDef(console_task, osPriorityNormal, 4*1024);
    p_rda_console.thread_id = osThreadCreate(osThread(console_task), NULL);
}


