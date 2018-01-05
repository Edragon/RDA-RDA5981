#ifndef _AT_H_
#define _AT_H_

//#define RDA_AT_DEBUG
#ifdef RDA_AT_DEBUG
#define RDA_AT_PRINT(fmt, ...) do {\
            printf(fmt, ##__VA_ARGS__);\
    } while (0)
#else
#define RDA_AT_PRINT(fmt, ...)
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

#endif
