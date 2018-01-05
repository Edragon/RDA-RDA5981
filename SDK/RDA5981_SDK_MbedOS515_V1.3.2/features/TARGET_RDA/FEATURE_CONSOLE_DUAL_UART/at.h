#ifndef _AT_H_
#define _AT_H_

#ifdef __cplusplus
extern "C" {
#endif

#define RDA_AT_DEBUG
#ifdef RDA_AT_DEBUG
#define RDA_AT_PRINT(fmt, ...) do {\
            printf(fmt, ##__VA_ARGS__);\
    } while (0)

#define RDA_AT_PRINT_UART0(fmt, ...) do {\
            console_uart0_printf(fmt, ##__VA_ARGS__);\
    } while (0)

#define AT_PRINT(idx, fmt, ...) do { \
        if(idx == UART1_IDX) { \
            printf(fmt, ##__VA_ARGS__);} else { \
            console_uart0_printf(fmt, ##__VA_ARGS__);} \
    } while(0)

#else
#define RDA_AT_PRINT(fmt, ...)
#define RDA_AT_PRINT_UART0(fmt, ...)
#define AT_PRINT(idx, fmt, ...)
#endif

typedef enum{
    ERROR_OK    =    0,
    ERROR_CMD   =   -1,
    ERROR_ABORT =   -2,
    ERROR_FAILE =   -3,
    ERROR_ARG =     -4,
}ERR_TYPE;

#define RESP(fmt, ...)          printf(fmt, ##__VA_ARGS__)
#define RESP_OK()               printf("+ok\r\n")
#define RESP_OK_EQU(fmt, ...)   printf("+ok=" fmt, ##__VA_ARGS__)
#define RESP_ERROR(err_type)    printf("+error=%d\r\n", err_type)

#define RESP_UART0(fmt, ...)          console_uart0_printf(fmt, ##__VA_ARGS__)
#define RESP_OK_UART0()               console_uart0_printf("+ok\r\n")
#define RESP_OK_EQU_UART0(fmt, ...)   console_uart0_printf("+ok=" fmt, ##__VA_ARGS__)
#define RESP_ERROR_UART0(err_type)    console_uart0_printf("+error=%d\r\n", err_type)

#define AT_RESP(idx, fmt, ...)  do { if(idx == UART1_IDX) { \
                                     printf(fmt, ##__VA_ARGS__); } else { \
                                     console_uart0_printf(fmt, ##__VA_ARGS__);}} while(0)

#define AT_RESP_OK(idx)         do { if(idx == UART1_IDX) { \
                                     printf("+ok\r\n");} else { \
                                     console_uart0_printf("+ok\r\n");}} while(0)

#define AT_RESP_OK_EQU(idx, fmt, ...)   do { if(idx == UART1_IDX) { \
                                        printf("+ok=" fmt, ##__VA_ARGS__);} else {\
                                        console_uart0_printf("+ok=" fmt, ##__VA_ARGS__);}} while(0)

#define AT_RESP_ERROR(idx, err_type)    do { if(idx == UART1_IDX) { \
                                        printf("+error=%d\r\n", err_type); } else {\
                                        console_uart0_printf("+error=%d\r\n", err_type);}}while(0)

#ifdef __cplusplus
}
#endif

#endif
