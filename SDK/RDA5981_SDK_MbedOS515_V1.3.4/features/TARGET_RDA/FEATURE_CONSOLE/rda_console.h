/*****************************************************************************
* @file     rda_console.h
* @date     26.07.2016
* @note
*
*****************************************************************************/
#ifndef _RDA_CONSOLE_H
#define _RDA_CONSOLE_H

#ifdef __cplusplus
extern "C" {
#endif

#define CMD_CBSIZE       128
#define CMD_PROMPT       "mbed91h> "
#define CMD_MAXARGS      16
#define CMD_LIST_COUNT   60
#define CONSOLE_RING_BUFFER_SIZE  2048

typedef struct {
    unsigned char *buffer;  /* the buffer holding the data              */
    unsigned int size;      /* the size of the allocated buffer         */
    unsigned int in;        /* data is added at offset (in % size)      */
    unsigned int out;       /* data is extracted from off. (out % size) */
} kfifo;

typedef struct cmd_tbl_s {
    char    *name;                                     /* Command Name */
    int     maxargs;                                   /* maximum number of arguments */
    int     (*cmd)(struct cmd_tbl_s *, int, char *[]);
    char    *usage;                                    /* Usage message(short)*/
} cmd_tbl_t;

extern cmd_tbl_t cmd_list[CMD_LIST_COUNT];
extern unsigned int cmd_cntr;

extern void init_console_irq_buffer(void);
extern unsigned int kfifo_put(unsigned char *buffer, unsigned int len);
extern unsigned int kfifo_get(unsigned char *buffer, unsigned int len);
extern unsigned int kfifo_len(void);
extern int run_command (char *cmd);
extern int handle_char (const char c, char *prompt);
extern uint32_t simple_strtoul(const char *cp,char **endp,unsigned int base);
extern void show_cmd_usage(const cmd_tbl_t *cmd);
extern int add_cmd_to_list(const cmd_tbl_t *cmd);

#ifdef __cplusplus
}
#endif

#endif

