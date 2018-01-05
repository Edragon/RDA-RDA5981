
#ifndef _USBDDBG_H
#define _USBDDBG_H

#include <stdio.h>

extern int usbd_dbg_level;
typedef enum {
    USBD_NONE_LEVEL  = 0,
    USBD_ERROR_LEVEL = 1,
    USBD_INFO_LEVEL  = 2,
    USBD_DEBUG_LEVEL = 3
} USBD_DEBUG_LEVEL_T;

#define USBD_DBG(level, fmt, ...) do {\
        if ((level <= usbd_dbg_level)) {\
            printf(fmt, ##__VA_ARGS__);\
        } \
    } while (0)

#define USBD_ERR(fmt, ...) do {\
            printf("[USBD_ERR]: " fmt, ##__VA_ARGS__);\
    } while (0)

#endif
