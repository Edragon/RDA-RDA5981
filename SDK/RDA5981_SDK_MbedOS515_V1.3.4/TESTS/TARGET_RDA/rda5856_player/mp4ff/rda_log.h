#ifndef RDA_LOG_H
#define RDA_LOG_H

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */

#include <stdio.h>

#ifdef M4A_DEBUG
#define M4ALOG(fmt, ...)   printf(fmt, ##__VA_ARGS__)
#else
#define M4ALOG(fmt, ...)
#endif

#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif
