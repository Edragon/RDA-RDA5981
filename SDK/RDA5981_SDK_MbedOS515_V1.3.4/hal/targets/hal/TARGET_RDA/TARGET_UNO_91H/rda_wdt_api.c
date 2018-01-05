/* mbed Microcontroller Library
 * Copyright (c) 2006-2013 ARM Limited
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */
#include <stddef.h>
#include "rda_wdt_api.h"
#include "rda_ccfg_api.h"

/** WDT HAL macro
 */
#define WDT_EN_BIT                  (9)
#define WDT_CLR_BIT                 (10)
#define WDT_TMRCNT_OFST             (11)
#define WDT_TMRCNT_WIDTH            (4)

/** WDT api functions
 */
void rda_wdt_init(wdt_t *obj, uint8_t to)
{
    uint32_t reg = 0;

    rda_ccfg_wdt_en();

    MBED_ASSERT(0x00U == (to & 0xF0U));
    obj->wdt = (RDA_WDT_TypeDef *)WDT_0;

    reg = obj->wdt->WDTCFG & ~(((0x01UL << WDT_TMRCNT_WIDTH) - 0x01UL) << WDT_TMRCNT_OFST);
    obj->wdt->WDTCFG = reg | (to << WDT_TMRCNT_OFST);
}

void rda_wdt_start(wdt_t *obj)
{
    obj->wdt->WDTCFG |= (0x01UL << WDT_EN_BIT);
}

void rda_wdt_stop(wdt_t *obj)
{
    obj->wdt->WDTCFG &= ~(0x01UL << WDT_EN_BIT);
}

void rda_wdt_feed(wdt_t *obj)
{
    obj->wdt->WDTCFG |= (0x01UL << WDT_CLR_BIT);
}

void rda_wdt_softreset(void)
{
    wdt_t obj;
    rda_ccfg_ckrst();
    rda_wdt_init(&obj, 0U);
    rda_wdt_start(&obj);
}

void rda_sys_softreset(void)
{
    rda_ccfg_ckrst();
    rda_ccfg_perrst();
    /* Ensure all outstanding memory accesses included buffered write are completed before reset */
    __DSB();
    SCB->AIRCR = (uint32_t)((0x5FAUL << SCB_AIRCR_VECTKEY_Pos)    |
                            (SCB->AIRCR & SCB_AIRCR_PRIGROUP_Msk) | /* Keep priority group unchanged */
                             SCB_AIRCR_VECTRESET_Msk);
    /* Ensure completion of memory access */
    __DSB();
    /* wait until reset */
    while(1) { __NOP(); }
}

