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
#include "rda_i2s_api.h"

#define RDA_EXIF_INTST      (RDA_EXIF->MISCSTCFG)
#define RDA_EXIF_IRQn       (EXIF_IRQn)

static uint8_t is_exif_irq_set = 0;

static void rda_exif_isr(void)
{
    uint32_t int_status = RDA_EXIF_INTST & 0xFFFF0000;
    if(int_status & 0x00FC0000) {
        rda_i2s_irq_handler(int_status);
    }
}

void rda_exif_irq_set(void)
{
    if(0 == is_exif_irq_set) {
        is_exif_irq_set = 1;
        NVIC_SetVector(RDA_EXIF_IRQn, (uint32_t)rda_exif_isr);
        NVIC_SetPriority(RDA_EXIF_IRQn, 0x1FUL);
        NVIC_EnableIRQ(RDA_EXIF_IRQn);
    }
}

