/*****************************************************************************
* @file     cmd_basic.c
* @date     26.07.2016
* @note
*
******************************************************************************/
#include <stdio.h>
#include <stdint.h>
#include <string.h>
#include "rda_console.h"
#include "at.h"

static int isdigit(unsigned char c)
{
    return ((c >= '0') && (c <='9'));
}

static int isxdigit(unsigned char c)
{
    if ((c >= '0') && (c <='9'))
        return 1;
    if ((c >= 'a') && (c <='f'))
        return 1;
    if ((c >= 'A') && (c <='F'))
        return 1;
    return 0;
}

static int islower(unsigned char c)
{
    return ((c >= 'a') && (c <='z'));
}

static unsigned char toupper(unsigned char c)
{
    if (islower(c))
        c -= 'a'-'A';
    return c;
}

uint32_t simple_strtoul(const char *cp,char **endp,unsigned int base)
{
    uint32_t result = 0,value;

    if (*cp == '0') {
        cp++;
        if ((*cp == 'x') && isxdigit(cp[1])) {
            base = 16;
            cp++;
        }
        if (!base) {
            base = 8;
        }
    }
    if (!base) {
        base = 10;
    }
    while (isxdigit(*cp) && (value = isdigit(*cp) ? *cp-'0' : (islower(*cp)
            ? toupper(*cp) : *cp)-'A'+10) < base) {
        result = result*base + value;
        cp++;
    }
    if (endp)
        *endp = (char *)cp;
    return result;
}

int cmd_get_data_size(const char *arg, int default_size)
{
    /* Check for a size specification .b, .w or .l. */
    int len = strlen(arg);

    if((len > 2) && (arg[len - 2] == '.')) {
        switch(arg[len - 1]) {
            case 'b': return 1;
            case 'w': return 2;
            case 'l': return 4;
            case 's': return -2;
            default:  return -1;
        }
    }

    return default_size;
}

void show_cmd_usage(const cmd_tbl_t *cmd)
{
    printf("Usage:\r\n%s %d\r\n -%s\r\n", cmd->name, cmd->maxargs, cmd->usage);
}

int add_cmd_to_list(const cmd_tbl_t *cmd)
{
    cmd_tbl_t *tmp_cmd;

    if(CMD_LIST_COUNT <= cmd_cntr) {
        printf("No more cmds supported.\r\n");
        return -1;
    }

    tmp_cmd = &(cmd_list[cmd_cntr]);
    cmd_cntr++;
    memcpy((char *)tmp_cmd, (char *)cmd, sizeof(cmd_tbl_t));

    return 0;
}

int do_at( cmd_tbl_t *cmd, int argc, char *argv[])
{
    RESP_OK();
    return 0;
}

#if 0
int do_mem_md(cmd_tbl_t *cmd, int argc, char *argv[])
{
    static uint32_t last_addr = 0x00100000;
    static uint32_t last_cnt  = 0x01;        /* Set to 1 for backward-compatible  */
    static uint8_t  last_size = 4;           /* Set to 4 for backward-compatible  */

    uint32_t addr, cnt;
    int size;

    /* We use the last specified parameters, unless new ones are entered. */
    addr = last_addr;
    cnt  = last_cnt;
    size = last_size;

    if(argc < 2) {
        show_cmd_usage(cmd);
        return -1;
    }

    addr = simple_strtoul(argv[1], NULL, 16);

    if(argc > 2) {
        cnt = simple_strtoul(argv[2], NULL, 0);
    }

    if((size = cmd_get_data_size(argv[0], 4)) < 0) {
        printf("Unknown data size\n");
        show_cmd_usage(cmd);
        return -2;
    }

    /* Compatible with previous version which cnt always = 1 */
    if(cnt == 1) {
        if(size == 4) {
            printf("[0x%08lX] = 0x%08lX\r\n", addr & 0xFFFFFFFC,
                *((volatile uint32_t*)(addr & 0xFFFFFFFC)));
        } else if(size == 2) {
            printf("[0x%08lX] = 0x%04X\r\n", addr & 0xFFFFFFFE,
                *((volatile uint16_t*)(addr & 0xFFFFFFFE)));
        } else if(size == 1) {
            printf("[0x%08lX] = 0x%02X\r\n", addr,
                *((volatile uint8_t*)(addr)));
        }

        return 0;
    }

//    dump_mem(addr, cnt, size, true, false, false);

    return 0;
}

int do_mem_mw( cmd_tbl_t *cmd, int argc, char *argv[])
{
    uint32_t addr;
    uint32_t value;

    if ((argc < 3)) {
        show_cmd_usage(cmd);
        return -1;
    }

    addr = simple_strtoul(argv[1], NULL, 16);
    printf("Addr  = 0x%08lX\r\n", addr);

    value = simple_strtoul(argv[2], NULL, 16);
    printf("Value = 0x%08lX\r\n", value);

    *((volatile uint32_t *)addr) = value;
    printf("Write Done\r\n");

    return 0;
}
#endif


