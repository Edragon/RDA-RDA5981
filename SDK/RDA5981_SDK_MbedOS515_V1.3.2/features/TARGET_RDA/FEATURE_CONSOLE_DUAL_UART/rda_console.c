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

static unsigned char console_recv_ring_buffer_0[CONSOLE_RING_BUFFER_SIZE] = {0};
static kfifo console_kfifo_0;
char console_buffer_0[CMD_CBSIZE];
char lastcommand_0[CMD_CBSIZE] = { 0, };

#define ZEROPAD 1       /* pad with zero */
#define SIGN    2       /* unsigned/signed long */
#define PLUS    4       /* show plus */
#define SPACE   8       /* space if plus */
#define LEFT    16      /* left justified */
#define SPECIAL 32      /* 0x */
#define LARGE   64      /* use 'ABCDEF' instead of 'abcdef' */

#define is_digit(c)     ((c) >= '0' && (c) <= '9')

//extern int do_help ( cmd_tbl_t *cmd, int argc, char *argv[]);
//extern int do_mem_md(cmd_tbl_t *cmd, int argc, char *argv[]);
//extern int do_mem_mw( cmd_tbl_t *cmd, int argc, char *argv[]);
extern int do_at( cmd_tbl_t *cmd, int argc, char *argv[]);
extern void console_puts(const char *s, unsigned char idx);
extern void console_putc(char c, unsigned char idx);
extern void console_uart0_printf(const char *fmt, ...);
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
__STATIC_INLINE void kfifo_reset(unsigned char idx)
{
    core_util_critical_section_enter();

	if(idx == UART1_IDX)
    	__kfifo_reset(&console_kfifo);
	else
		__kfifo_reset(&console_kfifo_0);

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
unsigned int kfifo_put(unsigned char *buffer, unsigned int len, unsigned char idx)
{
    unsigned int ret;

    core_util_critical_section_enter();

	if(idx == UART1_IDX)
    	ret = __kfifo_put(&console_kfifo, buffer, len);
	else
		ret = __kfifo_put(&console_kfifo_0, buffer, len);

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
unsigned int kfifo_get(unsigned char *buffer, unsigned int len, unsigned char idx)
{
    unsigned int ret;
    kfifo *fifo;

    core_util_critical_section_enter();

	if(idx == UART1_IDX)
    	fifo = &console_kfifo;
	else
		fifo = &console_kfifo_0;

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
unsigned int kfifo_len(unsigned char idx)
{
    unsigned int ret;

    core_util_critical_section_enter();

	if(idx == UART1_IDX)
    	ret = __kfifo_len(&console_kfifo);
	else
		ret = __kfifo_len(&console_kfifo_0);

    core_util_critical_section_exit();

    return ret;
}

void init_console_irq_buffer(unsigned char idx)
{
	if(idx == UART1_IDX)
    	kfifo_init(&console_kfifo, console_recv_ring_buffer, CONSOLE_RING_BUFFER_SIZE);
	else
		kfifo_init(&console_kfifo_0, console_recv_ring_buffer_0, CONSOLE_RING_BUFFER_SIZE);
}

static int parse_line (char *line, char *argv[], unsigned char idx)
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

    AT_RESP_ERROR(idx, ERROR_ARG);

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


int run_command(char *cmd, unsigned char idx)
{
    cmd_tbl_t *cmdtp;
    char *argv[CMD_MAXARGS + 1];    /* NULL terminated    */
    int argc;

    /* Extract arguments */
    if ((argc = parse_line(cmd, argv, idx)) == 0) {
        return -1;
    }

    /* Look up command in command table */
    if ((cmdtp = find_cmd(argv[0])) == 0) {
        AT_RESP_ERROR(idx, ERROR_CMD);
        return -1;
    }

    /* found - check max args */
    if (argc > cmdtp->maxargs) {
        AT_RESP_ERROR(idx, ERROR_ARG);
		return -1;
    }

    /* OK - call function to do the command */
    if ((cmdtp->cmd) (cmdtp, argc, argv, idx) != 0) {
        return -1;
    }

    return 0;
}

static char *delete_char(char *buffer, char *p, int *colp, int *np, int plen, unsigned char idx)
{
    char *s;

    if (*np == 0) {
        return (p);
    }

    if (*(--p) == '\t') {            /* will retype the whole line    */
        while (*colp > plen) {
            console_puts(erase_seq, idx);
            (*colp)--;
        }
        for (s=buffer; s<p; ++s) {
            if (*s == '\t') {
                console_puts(tab_seq+((*colp) & 07), idx);
                *colp += 8 - ((*colp) & 07);
            } else {
                ++(*colp);
                console_putc(*s, idx);
            }
        }
    } else {
        console_puts(erase_seq, idx);
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
            console_puts(prompt, UART1_IDX);

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
            console_puts("\r\n", UART1_IDX);
            return (p - console_buffer);

        case '\0':                /* nul            */
            return -1;

        case 0x03:                /* ^C - break        */
            console_buffer[0] = '\0';    /* discard input */
            return 0;

        case 0x15:                /* ^U - erase line    */
            while (col > plen) {
                console_puts(erase_seq, UART1_IDX);
                --col;
            }
            p = console_buffer;
            n = 0;
            return -1;

        case 0x17:                /* ^W - erase word     */
            p=delete_char(console_buffer, p, &col, &n, plen, UART1_IDX);
            while ((n > 0) && (*p != ' ')) {
                p=delete_char(console_buffer, p, &col, &n, plen, UART1_IDX);
            }
            return -1;

        case 0x08:                /* ^H  - backspace    */
        case 0x7F:                /* DEL - backspace    */
            p=delete_char(console_buffer, p, &col, &n, plen, UART1_IDX);
            return -1;

        default:
         /*  Must be a normal character then  */
            if (n < CMD_CBSIZE-2) {
                if (c == '\t') {    /* expand TABs        */
                    console_puts(tab_seq+(col&07), UART1_IDX);
                    col += 8 - (col&07);
                } else {
                    if(echo_flag == 1){
                        ++col;         /* echo input        */
                        console_putc(c, UART1_IDX);
                    }
                }
                *p++ = c;
                ++n;
            } else {          /* Buffer full        */
                console_putc('\a',UART1_IDX);
            }
    }

    return -1;
}

int handle_char_uart0(const char c, char *prompt) {
    static char   *p   = console_buffer_0;
    static int    n    = 0;              /* buffer index        */
    static int    plen = 0;           /* prompt length    */
    static int    col;                /* output column cnt    */

    if (prompt) {
        plen = strlen(prompt);
        if(plen == 1 && prompt[0] == 'r')
            plen = 0;
        else
            console_puts(prompt, UART0_IDX);
        p = console_buffer_0;
        n = 0;
        return 0;
    }
    col = plen;

    /* Special character handling */
    switch (c) {
        case '\r':                /* Enter        */
        case '\n':
            *p = '\0';
            console_puts("\r\n", UART0_IDX);
            return (p - console_buffer_0);

        case '\0':                /* nul            */
            return -1;

        case 0x03:                /* ^C - break        */
            console_buffer_0[0] = '\0';    /* discard input */
            return 0;

        case 0x15:                /* ^U - erase line    */
            while (col > plen) {
                console_puts(erase_seq, UART0_IDX);
                --col;
            }
            p = console_buffer_0;
            n = 0;
            return -1;

        case 0x17:                /* ^W - erase word     */
            p=delete_char(console_buffer_0, p, &col, &n, plen, UART0_IDX);
            while ((n > 0) && (*p != ' ')) {
                p=delete_char(console_buffer_0, p, &col, &n, plen, UART0_IDX);
            }
            return -1;

        case 0x08:                /* ^H  - backspace    */
        case 0x7F:                /* DEL - backspace    */
            p=delete_char(console_buffer_0, p, &col, &n, plen, UART0_IDX);
            return -1;

        default:
         /*  Must be a normal character then  */
            if (n < CMD_CBSIZE-2) {
                if (c == '\t') {    /* expand TABs        */
                    console_puts(tab_seq+(col&07), UART0_IDX);
                    col += 8 - (col&07);
                } else {
                    if(echo_flag == 1){
                        ++col;         /* echo input        */
                        console_putc(c,UART0_IDX);
                    }
                }
                *p++ = c;
                ++n;
            } else {          /* Buffer full        */
                console_putc('\a',UART0_IDX);
            }
    }

    return -1;
}

static int skip_atoi(const char **s)
{
    int i=0;

    while (is_digit(**s))
        i = i*10 + *((*s)++) - '0';
    return i;
}

static char * number(char * str, long num, int base, int size, int precision ,int type)
{
    char c,sign,tmp[66];
    const char *digits="0123456789abcdefghijklmnopqrstuvwxyz";
    int i;

    if (type & LARGE)
        digits = "0123456789ABCDEFGHIJKLMNOPQRSTUVWXYZ";
    if (type & LEFT)
        type &= ~ZEROPAD;
    if (base < 2 || base > 36)
        return 0;
    c = (type & ZEROPAD) ? '0' : ' ';
    sign = 0;
    if (type & SIGN) {
        if (num < 0) {
            sign = '-';
            num = -num;
            size--;
        } else if (type & PLUS) {
            sign = '+';
            size--;
        } else if (type & SPACE) {
            sign = ' ';
            size--;
        }
    }
    if (type & SPECIAL) {
        if (base == 16)
            size -= 2;
        else if (base == 8)
            size--;
    }
    i = 0;
    if (num == 0)
        tmp[i++]='0';
    else while (num != 0) {
        tmp[i++] = digits[(((unsigned long) num) % (unsigned) base)];
        num = ((unsigned long) num) / (unsigned) base;
    }
    if (i > precision)
        precision = i;
    size -= precision;
    if (!(type&(ZEROPAD+LEFT)))
        while(size-->0)
            *str++ = ' ';
    if (sign)
        *str++ = sign;
    if (type & SPECIAL) {
        if (base==8)
            *str++ = '0';
        else if (base==16) {
            *str++ = '0';
            *str++ = digits[33];
        }
    }
    if (!(type & LEFT))
        while (size-- > 0)
            *str++ = c;
    while (i < precision--)
        *str++ = '0';
    while (i-- > 0)
        *str++ = tmp[i];
    while (size-- > 0)
        *str++ = ' ';
    return str;
}

static int strnlen(const char * s, int count)
{
    const char *sc;

    for (sc = s; count-- && *sc != '\0'; ++sc)
        /* nothing */;

    return sc - s;
}

int rda_vsprintf(char *buf, const char *fmt, va_list args)
{
	int len;
	unsigned long num;
	int i, base;
	char * str;
	const char *s;

	int flags;		  /* flags to number() */

	int field_width;	/* width of output field */
	int precision;		  /* min. # of digits for integers; max
				   number of chars for from string */
	int qualifier;		  /* 'h', 'l', or 'q' for integer fields */

	for (str=buf ; *fmt ; ++fmt) {
		if (*fmt != '%') {
			*str++ = *fmt;
			continue;
		}

		/* process flags */
		flags = 0;
		repeat:
			++fmt;		  /* this also skips first '%' */
			switch (*fmt) {
				case '-': flags |= LEFT; goto repeat;
				case '+': flags |= PLUS; goto repeat;
				case ' ': flags |= SPACE; goto repeat;
				case '#': flags |= SPECIAL; goto repeat;
				case '0': flags |= ZEROPAD; goto repeat;
				}

		/* get field width */
		field_width = -1;
		if (is_digit(*fmt))
			field_width = skip_atoi(&fmt);
		else if (*fmt == '*') {
			++fmt;
			/* it's the next argument */
			field_width = va_arg(args, int);
			if (field_width < 0) {
				field_width = -field_width;
				flags |= LEFT;
			}
		}

		/* get the precision */
		precision = -1;
		if (*fmt == '.') {
			++fmt;
			if (is_digit(*fmt))
				precision = skip_atoi(&fmt);
			else if (*fmt == '*') {
				++fmt;
				/* it's the next argument */
				precision = va_arg(args, int);
			}
			if (precision < 0)
				precision = 0;
		}

		/* get the conversion qualifier */
		qualifier = -1;
		if (*fmt == 'h' || *fmt == 'l' || *fmt == 'q') {
			qualifier = *fmt;
			++fmt;
		}

		/* default base */
		base = 10;

		switch (*fmt) {
		case 'c':
			if (!(flags & LEFT))
				while (--field_width > 0)
					*str++ = ' ';
			*str++ = (unsigned char) va_arg(args, int);
			while (--field_width > 0)
				*str++ = ' ';
			continue;

		case 's':
			s = va_arg(args, char *);
			if (!s)
				s = "<NULL>";

			len = strnlen(s, precision);

			if (!(flags & LEFT))
				while (len < field_width--)
					*str++ = ' ';
			for (i = 0; i < len; ++i)
				*str++ = *s++;
			while (len < field_width--)
				*str++ = ' ';
			continue;
		case 'T':
		{
			continue;
		}
		case 'p':
			if (field_width == -1) {
				field_width = 2*sizeof(void *);
				flags |= ZEROPAD;
			}
			str = number(str,
				(unsigned long) va_arg(args, void *), 16,
				field_width, precision, flags);
			continue;


		case 'n':
			if (qualifier == 'l') {
				long * ip = va_arg(args, long *);
				*ip = (str - buf);
			} else {
				int * ip = va_arg(args, int *);
				*ip = (str - buf);
			}
			continue;

		case '%':
			*str++ = '%';
			continue;

		/* integer number formats - set up the flags and "break" */
		case 'o':
			base = 8;
			break;

		case 'X':
			flags |= LARGE;
		case 'x':
			base = 16;
			break;

		case 'd':
		case 'i':
			flags |= SIGN;
		case 'u':
			break;

		default:
			*str++ = '%';
			if (*fmt)
				*str++ = *fmt;
			else
				--fmt;
			continue;
		}
		if (qualifier == 'l')
			num = va_arg(args, unsigned long);
		else if (qualifier == 'h') {
			num = (unsigned short) va_arg(args, int);
			if (flags & SIGN)
				num = (short) num;
		} else if (flags & SIGN)
			num = va_arg(args, int);
		else
			num = va_arg(args, unsigned int);
		str = number(str, num, base, field_width, precision, flags);
	}
	*str = '\0';
	return str-buf;
}

