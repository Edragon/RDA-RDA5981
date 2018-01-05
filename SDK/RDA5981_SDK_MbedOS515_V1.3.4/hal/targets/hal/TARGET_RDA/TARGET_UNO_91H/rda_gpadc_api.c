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

#include "pinmap.h"
#include "mbed_error.h"
//#include "cmsis.h"
#include "rda_gpadc_api.h"
#include "rda_ccfg_api.h"
#include "gpio_api.h"

#if (DEVICE_INTERRUPTIN && DEVICE_ANALOGIN)

static gpadc_handler irq_handler;
static const PinMap PinMap_GPADC[] = {
    {PB_6, GPADC0_0, 0},
    {PB_7, GPADC0_1, 0},
    {NC,   NC,       0}
};
static uint8_t irq_mode[GPADC0_1 + 1][GP_FALL + 1] = {{0,},};

extern void analogin_free(analogin_t *obj);

static void gpadc_interrupt_handler(uint32_t id, gpio_irq_event event) {
    gpadc_t *obj = (gpadc_t *)id;
    uint16_t val = 0x3FFU, tmpval;

    if(IRQ_FALL == event) {
        analogin_init(&(obj->ain), obj->gpadc.pin);
        val = analogin_read_u16(&(obj->ain));
        //analogin_free(&(obj->ain));
        //analogin_init(&(obj->ain), obj->gpadc.pin);
        tmpval = analogin_read_u16(&(obj->ain));
        analogin_free(&(obj->ain));

        val = (tmpval < val) ? tmpval : val;  // select the smaller
        if(val >= 0x3FFU) {
            return;
        }
    }

    irq_handler((uint8_t)(obj->gpadc.ch), val, (gpadc_event)event);
}

void rda_gpadc_init(gpadc_t *obj, PinName pin, gpadc_handler handler) {
    MBED_ASSERT(pin != NC);
    obj->gpadc.ch = (GPADCName)pinmap_peripheral(pin, PinMap_GPADC);
    obj->gpadc.pin = pin;

    irq_handler = handler;
    gpio_irq_init(&(obj->irq), pin, gpadc_interrupt_handler, (uint32_t)obj);
    gpio_init_in(&(obj->gpio), pin);
}

void rda_gpadc_set(gpadc_t *obj, gpadc_event event, uint32_t enable) {
    MBED_ASSERT(obj->gpadc.ch <= GPADC0_1);
    if(0 == irq_mode[(int)(obj->gpadc.ch)][(int)(event)]) {
        gpio_irq_set(&(obj->irq), (gpio_irq_event)event, enable);
        irq_mode[(int)(obj->gpadc.ch)][(int)(event)] = 1;
    }
}

void rda_gpadc_free(gpadc_t *obj) {
    gpio_irq_free(&(obj->irq));
    irq_mode[(int)(obj->gpadc.ch)][(int)GP_RISE] = 0;
    irq_mode[(int)(obj->gpadc.ch)][(int)GP_FALL] = 0;
}
#endif /* DEVICE_INTERRUPTIN && DEVICE_ANALOGIN */

