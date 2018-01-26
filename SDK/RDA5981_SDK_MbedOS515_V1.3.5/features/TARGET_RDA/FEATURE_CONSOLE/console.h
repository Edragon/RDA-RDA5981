/*****************************************************************************
* @file     console.h
* @date     26.07.2016
* @note
*
*****************************************************************************/
#ifndef _CONSOLE_H
#define _CONSOLE_H

#include <stdint.h>
#include <stdio.h>
#include <stddef.h>
#include <string.h>
#include "cmsis_os.h"
#include "rda_console.h"
#include "critical.h"

//#define CONSOLE_DUMP_STACK_EN

#ifdef __cplusplus
extern "C" {
#endif

typedef struct console_sema {
    osSemaphoreId    sema_id;
    osSemaphoreDef_t def;
#ifdef CMSIS_OS_RTX
    uint32_t         data[2];
#endif
} console_sema_t;

typedef struct rda_console {
    uint8_t        cmd_cnt;
    osThreadId     thread_id;
    console_sema_t console_sema;
} rda_console_t;

extern void console_init(void);
extern void console_puts(const char *s);
extern void console_putc(char c);
extern void console_set_baudrate(unsigned int baudrate);

#define console_strtoul     simple_strtoul
#define console_cmd_usage   show_cmd_usage
#define console_cmd_add     add_cmd_to_list
#define console_fifo_len    kfifo_len
#define console_fifo_get    kfifo_get

#ifdef __cplusplus
}
#endif


#endif

