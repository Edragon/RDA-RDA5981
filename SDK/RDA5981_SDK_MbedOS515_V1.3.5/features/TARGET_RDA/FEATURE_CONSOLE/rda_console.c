/*****************************************************************************
* @file     rda_console.c
* @date     26.07.2016
* @note
*
*****************************************************************************/
#include <stdint.h>
#include <stdio.h>
#include <stddef.h>
#include <string.h>
#include "critical.h"
#include "rda_console.h"
#include "at.h"

#if  !defined ( __STATIC_INLINE )
#if   defined ( __CC_ARM )
  #define __STATIC_INLINE  static __inline
#elif defined ( __GNUC__ )
  #define __STATIC_INLINE  static inline
#elif defined ( __ICCARM__ )
  #define __STATIC_INLINE  static inline
#else
  #error "Unknown compiler"
#endif
#endif

static char erase_seq[] = "\b \b";       /* erase sequence         */
static char   tab_seq[] = "    ";        /* used to expand TABs    */
static unsigned char console_recv_ring_buffer[CONSOLE_RING_BUFFER_SIZE] = {0};
static kfifo console_kfifo;
char console_buffer[CMD_CBSIZE];
char lastcommand[CMD_CBSIZE] = { 0, };
unsigned int echo_flag = 1;

//extern int do_help ( cmd_tbl_t *cmd, int argc, char *argv[]);
//extern int do_mem_md(cmd_tbl_t *cmd, int argc, char *argv[]);
//extern int do_mem_mw( cmd_tbl_t *cmd, int argc, char *argv[]);
extern int do_at( cmd_tbl_t *cmd, int argc, char *argv[]);
extern void console_puts(const char *s);
extern void console_putc(char c);
/*
cmd_tbl_t cmd_list[CMD_LIST_COUNT] = {
    {
        "help",     3,   do_help,
        "help       - help info\n"
    },
    {
        "md",       3,   do_mem_md,
        "md        - memory display\n"
    },
    {
        "mw",       3,   do_mem_mw,
        "mw       - memory write (fill)\n"
    },

};
unsigned int cmd_cntr = 3U;
*/
cmd_tbl_t cmd_list[CMD_LIST_COUNT] = {
    {
        "AT",               1,   do_at,
        "AT                 - AT mode"
    },
};
unsigned int cmd_cntr = 1;

#define min(a,b) (((a) < (b)) ? (a) : (b))

/**
 * kfifo_init - allocates a new FIFO using a preallocated buffer
 * @buffer: the preallocated buffer to be used.
 * @size: the size of the internal buffer, this have to be a power of 2.
 * @gfp_mask: get_free_pages mask, passed to kmalloc()
 * @lock: the lock to be used to protect the fifo buffer
 *
 * Do NOT pass the kfifo to kfifo_free() after use! Simply free the
 * &struct kfifo with kfree().
 */
int kfifo_init(kfifo *fifo, unsigned char *buffer, unsigned int size)
{
    core_util_critical_section_enter();
    /* size must be a power of 2 */
    if((size & (size - 1)) != 0)
        return -1;

    if (!buffer)
        return -1;

    fifo->buffer = buffer;
    fifo->size = size;
    fifo->in = fifo->out = 0;
    core_util_critical_section_exit();

    return 0;
}

/**
 * __kfifo_reset - removes the entire FIFO contents, no locking version
 * @fifo: the fifo to be emptied.
 */
__STATIC_INLINE void __kfifo_reset(kfifo *fifo)
{
    fifo->in = fifo->out = 0;
}

/**
 * kfifo_reset - removes the entire FIFO contents
 * @fifo: the fifo to be emptied.
 */
__STATIC_INLINE void kfifo_reset(void)
{
    core_util_critical_section_enter();

    __kfifo_reset(&console_kfifo);

    core_util_critical_section_exit();
}

/**
 * __kfifo_put - puts some data into the FIFO, no locking version
 * @fifo: the fifo to be used.
 * @buffer: the data to be added.
 * @len: the length of the data to be added.
 *
 * This function copies at most @len bytes from the @buffer into
 * the FIFO depending on the free space, and returns the number of
 * bytes copied.
 *
 * Note that with only one concurrent reader and one concurrent
 * writer, you don't need extra locking to use these functions.
 */
unsigned int __kfifo_put(kfifo *fifo,
            unsigned char *buffer, unsigned int len)
{
    unsigned int l;

    len = min(len, fifo->size - fifo->in + fifo->out);

    /*
     * Ensure that we sample the fifo->out index -before- we
     * start putting bytes into the kfifo.
     */

    /* first put the data starting from fifo->in to buffer end */
    l = min(len, fifo->size - (fifo->in & (fifo->size - 1)));
    memcpy(fifo->buffer + (fifo->in & (fifo->size - 1)), buffer, l);

    /* then put the rest (if any) at the beginning of the buffer */
    memcpy(fifo->buffer, buffer + l, len - l);

    /*
     * Ensure that we add the bytes to the kfifo -before-
     * we update the fifo->in index.
     */

    fifo->in += len;

    return len;
}

/**
 * kfifo_put - puts some data into the FIFO
 * @fifo: the fifo to be used.
 * @buffer: the data to be added.
 * @len: the length of the data to be added.
 *
 * This function copies at most @len bytes from the @buffer into
 * the FIFO depending on the free space, and returns the number of
 * bytes copied.
 */
unsigned int kfifo_put(unsigned char *buffer, unsigned int len)
{
    unsigned int ret;

    core_util_critical_section_enter();

    ret = __kfifo_put(&console_kfifo, buffer, len);

    core_util_critical_section_exit();

    return ret;
}

/**
 * __kfifo_get - gets some data from the FIFO, no locking version
 * @fifo: the fifo to be used.
 * @buffer: where the data must be copied.
 * @len: the size of the destination buffer.
 *
 * This function copies at most @len bytes from the FIFO into the
 * @buffer and returns the number of copied bytes.
 *
 * Note that with only one concurrent reader and one concurrent
 * writer, you don't need extra locking to use these functions.
 */
static unsigned int __kfifo_get(kfifo *fifo,
            unsigned char *buffer, unsigned int len)
{
    unsigned int l;

    len = min(len, fifo->in - fifo->out);

    /*
     * Ensure that we sample the fifo->in index -before- we
     * start removing bytes from the kfifo.
     */

    /* first get the data from fifo->out until the end of the buffer */
    l = min(len, fifo->size - (fifo->out & (fifo->size - 1)));
    memcpy(buffer, fifo->buffer + (fifo->out & (fifo->size - 1)), l);

    /* then get the rest (if any) from the beginning of the buffer */
    memcpy(buffer + l, fifo->buffer, len - l);

    /*
     * Ensure that we remove the bytes from the kfifo -before-
     * we update the fifo->out index.
     */

    fifo->out += len;

    return len;
}

/**
 * kfifo_get - gets some data from the FIFO
 * @fifo: the fifo to be used.
 * @buffer: where the data must be copied.
 * @len: the size of the destination buffer.
 *
 * This function copies at most @len bytes from the FIFO into the
 * @buffer and returns the number of copied bytes.
 */
unsigned int kfifo_get(unsigned char *buffer, unsigned int len)
{
    unsigned int ret;
    kfifo *fifo;

    core_util_critical_section_enter();

    fifo = &console_kfifo;

    ret = __kfifo_get(fifo, buffer, len);

    /*
     * optimization: if the FIFO is empty, set the indices to 0
     * so we don't wrap the next time
     */
    if (fifo->in == fifo->out)
        fifo->in = fifo->out = 0;

    core_util_critical_section_exit();

    return ret;
}

/**
 * __kfifo_len - returns the number of bytes available in the FIFO, no locking version
 * @fifo: the fifo to be used.
 */
__STATIC_INLINE unsigned int __kfifo_len(kfifo *fifo)
{
    return fifo->in - fifo->out;
}

/**
 * kfifo_len - returns the number of bytes available in the FIFO
 * @fifo: the fifo to be used.
 */
unsigned int kfifo_len(void)
{
    unsigned int ret;

    core_util_critical_section_enter();

    ret = __kfifo_len(&console_kfifo);

    core_util_critical_section_exit();

    return ret;
}

void init_console_irq_buffer(void)
{
    kfifo_init(&console_kfifo, console_recv_ring_buffer, CONSOLE_RING_BUFFER_SIZE);
}

static int parse_line (char *line, char *argv[])
{
    int nargs = 0;

    while (nargs < CMD_MAXARGS) {
        /* skip any white space */
        while ((*line == ' ') || (*line == '\t') || (*line == ',')) {
            ++line;
        }

        if (*line == '\0') {    /* end of line, no more args    */
            argv[nargs] = 0;
            return (nargs);
        }

        /** miaodefang modified for RDA5991 VerG Begin */
        /* Argument include space should be bracketed by quotation mark */
        if(*line == '\"') {
            /* Skip quotation mark */
            line++;

            /* Begin of argument string */
            argv[nargs++] = line;

            /* Until end of argument */
            while(*line && (*line != '\"')) {
                ++line;
            }
        } else {
            argv[nargs++] = line;    /* begin of argument string    */

            /* find end of string */
            while(*line && (*line != ',') && (*line != '=')) {
                ++line;
            }
        }
        /** miaodefang modified for RDA5991 VerG End */

        if (*line == '\0') {    /* end of line, no more args    */
            argv[nargs] = 0;
            return (nargs);
        }

        *line++ = '\0';         /* terminate current arg     */
    }

    RESP_ERROR(ERROR_ARG);

    return (nargs);
}

static cmd_tbl_t *find_cmd (const char *cmd)
{
    cmd_tbl_t *cmdtp;
    cmd_tbl_t *cmdtp_temp = &cmd_list[0];    /* Init value */
    uint32_t len;
    int n_found = 0;
    int i;

    len = strlen(cmd);

    for (i = 0;i < (int)cmd_cntr;i++) {
        cmdtp = &cmd_list[i];
        if (strncmp(cmd, cmdtp->name, len) == 0) {
            if (len == strlen(cmdtp->name))
                return cmdtp;      /* full match */

            cmdtp_temp = cmdtp;    /* abbreviated command ? */
            n_found++;
        }
    }
    if (n_found == 1) {  /* exactly one match */
        return cmdtp_temp;
    }

    return 0;   /* not found or ambiguous command */
}


int run_command(char *cmd)
{
    cmd_tbl_t *cmdtp;
    char *argv[CMD_MAXARGS + 1];    /* NULL terminated    */
    int argc;

    /* Extract arguments */
    if ((argc = parse_line(cmd, argv)) == 0) {
        return -1;
    }

    /* Look up command in command table */
    if ((cmdtp = find_cmd(argv[0])) == 0) {
        RESP_ERROR(ERROR_CMD);
        return -1;
    }

    /* found - check max args */
    if (argc > cmdtp->maxargs) {
        RESP_ERROR(ERROR_ARG);
        return -1;
    }

    /* OK - call function to do the command */
    if ((cmdtp->cmd) (cmdtp, argc, argv) != 0) {
        return -1;
    }

    return 0;
}

static char *delete_char(char *buffer, char *p, int *colp, int *np, int plen)
{
    char *s;

    if (*np == 0) {
        return (p);
    }

    if (*(--p) == '\t') {            /* will retype the whole line    */
        while (*colp > plen) {
            console_puts(erase_seq);
            (*colp)--;
        }
        for (s=buffer; s<p; ++s) {
            if (*s == '\t') {
                console_puts(tab_seq+((*colp) & 07));
                *colp += 8 - ((*colp) & 07);
            } else {
                ++(*colp);
                console_putc(*s);
            }
        }
    } else {
        console_puts(erase_seq);
        (*colp)--;
    }
    (*np)--;
    return (p);
}


int handle_char(const char c, char *prompt) {
    static char   *p   = console_buffer;
    static int    n    = 0;              /* buffer index        */
    static int    plen = 0;           /* prompt length    */
    static int    col;                /* output column cnt    */

    if (prompt) {
        plen = strlen(prompt);
        if(plen == 1 && prompt[0] == 'r')
            plen = 0;
        else
            console_puts(prompt);
        p = console_buffer;
        n = 0;
        return 0;
    }
    col = plen;

    /* Special character handling */
    switch (c) {
        case '\r':                /* Enter        */
        case '\n':
            *p = '\0';
            console_puts("\r\n");
            return (p - console_buffer);

        case '\0':                /* nul            */
            return -1;

        case 0x03:                /* ^C - break        */
            console_buffer[0] = '\0';    /* discard input */
            return 0;

        case 0x15:                /* ^U - erase line    */
            while (col > plen) {
                console_puts(erase_seq);
                --col;
            }
            p = console_buffer;
            n = 0;
            return -1;

        case 0x17:                /* ^W - erase word     */
            p=delete_char(console_buffer, p, &col, &n, plen);
            while ((n > 0) && (*p != ' ')) {
                p=delete_char(console_buffer, p, &col, &n, plen);
            }
            return -1;

        case 0x08:                /* ^H  - backspace    */
        case 0x7F:                /* DEL - backspace    */
            p=delete_char(console_buffer, p, &col, &n, plen);
            return -1;

        default:
         /*  Must be a normal character then  */
            if (n < CMD_CBSIZE-2) {
                if (c == '\t') {    /* expand TABs        */
                    console_puts(tab_seq+(col&07));
                    col += 8 - (col&07);
                } else {
                    if(echo_flag == 1){
                        ++col;         /* echo input        */
                        console_putc(c);
                    }
                }
                *p++ = c;
                ++n;
            } else {          /* Buffer full        */
                console_putc('\a');
            }
    }

    return -1;
}



