#ifndef __WDT_H
#define __WDT_H

#include "GPIO.h"

typedef enum {
    WDT_0  = (int)RDA_WDT_BASE
} WDTName;

//WDT HAL structure
typedef struct _wdt_t 
{
    RDA_WDT_TypeDef *wdt;
}wdt_t;


#ifdef __cplusplus
extern "C" {
#endif

/** Initialize the wdt module
 *
 */
void rda_wdt_init(wdt_t *obj, uint8_t to);

/** Start the wdt module
 *
 */
void rda_wdt_start(wdt_t *obj);

/** Stop the wdt module
 *
 */
void rda_wdt_stop(wdt_t *obj);

/** Feed the watchdog
 *
 */
void rda_wdt_feed(wdt_t *obj);

/** Software reset using watchdog
 *
 */
void rda_wdt_softreset(void);

/** System software reset
 *
 */
void rda_sys_softreset(void);

/**@}*/

#ifdef __cplusplus
}
#endif

#endif
