#ifndef RDA58XX_DBG_H
#define RDA58XX_DBG_H

enum {
    D_ERROR_LEVEL = 0,
    D_WARNING_LEVEL = 1,
    D_NOTICE_LEVEL = 2,
    D_INFO_LEVEL = 3,
    D_DEBUG_LEVEL = 4,
    D_TRACE_LEVEL = 5,
};

#define codec_dbg_level D_DEBUG_LEVEL

#define CODEC_LOG(level, fmt, ...)  do {\
        int dbg_level = D_##level##_LEVEL;\
        if((dbg_level <= codec_dbg_level)){\
            printf(fmt, ##__VA_ARGS__);\
        }\
    } while (0)

#endif
