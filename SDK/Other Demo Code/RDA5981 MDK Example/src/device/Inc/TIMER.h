#ifndef __TICKER_H
#define __TICKER_H

#include "GPIO.h"
//#include "ticker_api.h"

typedef uint32_t timestamp_t;




#ifdef __cplusplus
extern "C" {
#endif

uint32_t us_ticker_read(void);
	
#ifdef __cplusplus
}
#endif

#endif
