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
#if DEVICE_PWMOUT
#include "mbed_assert.h"
#include "pwmout_api.h"
#include "gpio_api.h"
#include "cmsis.h"
#include "pinmap.h"

#define PWM_CLK_SRC_20MHZ       (20000000)
#define PWM_CLK_SRC_32KHZ       (32768)

#define PWM_CLKGATE_REG         (RDA_SCU->CLKGATE1)
#define PWM_CLKSRC_REG          (RDA_SCU->PWMCFG)
#define PWM_CLKDIV_REG          (RDA_PWM->CLKR)
#define EXIF_PWM_EN_REG         (RDA_EXIF->MISCCFG)

/* PORT ID, PWM ID, Pin function */
static const PinMap PinMap_PWM[] = {
    {PA_0, PWM_6, 4},
    {PA_1, PWM_3, 4},
    {PB_0, PWM_2, 4},
    {PB_1, PWM_7, 4},
    {PB_2, PWM_5, 4},
    {PB_3, PWM_4, 4},
    {PB_8, PWM_0, 4},
    {PC_1, PWM_1, 5},
    {PD_0, PWM_0, 4},
    {PD_1, PWM_1, 4},
    {PD_2, PWM_2, 4},
    {PD_3, PWM_3, 4},
    {NC,   NC,    0}
};

__IO uint32_t *PWM_MATCH[] = {
    &(RDA_EXIF->PWM0CFG),
    &(RDA_EXIF->PWM1CFG),
    &(RDA_EXIF->PWM2CFG),
    &(RDA_EXIF->PWM3CFG),
    &( RDA_PWM->PWTCFG ),
    &( RDA_PWM->LPGCFG ),
    &( RDA_PWM->PWL0CFG),
    &( RDA_PWM->PWL1CFG)
};

static uint8_t is_pwmout_started(pwmout_t* obj);
static void pwmout_start(pwmout_t* obj);
static void pwmout_stop(pwmout_t* obj);
static void pwmout_update_cfgreg(pwmout_t* obj);

void pwmout_init(pwmout_t* obj, PinName pin)
{
    uint32_t reg_val = 0U;

    /* determine the channel */
    PWMName pwm = (PWMName)pinmap_peripheral(pin, PinMap_PWM);
    MBED_ASSERT(pwm != (PWMName)NC);

    obj->channel = pwm;
    obj->CFGR = PWM_MATCH[pwm];
    obj->pin = pin;

    /* Enable PWM Clock-gating */
    PWM_CLKGATE_REG |= (0x01UL << 2);

    /* Init PWM clock source and divider */
    if(PWM_4 >= pwm) {
        /* Src: 20MHz, Divider: (3 + 1) */
        reg_val = PWM_CLKSRC_REG & ~0x0000FFUL;
        PWM_CLKSRC_REG = reg_val | (0x01UL << 7) | 0x03UL | (0x01UL << 24);
        obj->base_clk = (PWM_CLK_SRC_20MHZ >> 2) >> 1;
    } else if(PWM_5 == pwm) {
        /* Src: 32KHz, Divider: (0 + 1) */
        reg_val = PWM_CLKSRC_REG & ~0x00FF00UL;
        PWM_CLKSRC_REG = reg_val | (0x00UL << 8) | (0x01UL << 25);
        obj->base_clk = PWM_CLK_SRC_32KHZ >> 1;
    } else {
        /* Src: 32KHz, Divider: (0 + 1) */
        reg_val = PWM_CLKSRC_REG & ~0xFF0000UL;
        PWM_CLKSRC_REG = reg_val | (0x00UL << 16) | (0x01UL << 26);
        obj->base_clk = PWM_CLK_SRC_32KHZ >> 1;
    }

    // default to 20ms: standard for servos, and fine for e.g. brightness control
    pwmout_period_ms(obj, 20);
    pwmout_write    (obj, 0);

    // Wire pinout
    pinmap_pinout(pin, PinMap_PWM);
}

void pwmout_free(pwmout_t* obj)
{
    /* Disable PWM Clock-gating */
    PWM_CLKGATE_REG &= ~(0x01UL << 2);
}

void pwmout_write(pwmout_t* obj, float value)
{
    uint32_t ticks;

    /* Check if already started */
    if(is_pwmout_started(obj))
        pwmout_stop(obj);

    if (value < 0.0f) {
        value = 0.0;
    } else if (value > 1.0f) {
        value = 1.0;
    }

    /* Set channel match to percentage */
    ticks = (uint32_t)((float)(obj->period_ticks) * value);

    if (0 == ticks) {
        obj->pulsewidth_ticks = 0;
    } else {
        /* Update Hw reg */
        if(ticks != obj->pulsewidth_ticks) {
            obj->pulsewidth_ticks = ticks;
            pwmout_update_cfgreg(obj);
        }
    }
    /* Start PWM module */
    pwmout_start(obj);
}

float pwmout_read(pwmout_t* obj)
{
    float v = (float)(obj->pulsewidth_ticks) / (float)(obj->period_ticks);
    return (v > 1.0f) ? (1.0f) : (v);
}

void pwmout_period(pwmout_t* obj, float seconds)
{
    pwmout_period_us(obj, seconds * 1000000.0f);
}

void pwmout_period_ms(pwmout_t* obj, int ms)
{
    pwmout_period_us(obj, ms * 1000);
}

/* Set the PWM period, keeping the duty cycle the same. */
void pwmout_period_us(pwmout_t* obj, int us)
{
    uint32_t ticks;

    MBED_ASSERT(PWM_7 >= (PWMName)(obj->channel));

    /* Check if already started */
    if(is_pwmout_started(obj))
        pwmout_stop(obj);

    /* Calculate number of ticks */
    ticks = (uint64_t)obj->base_clk * us / 1000000;

    if(ticks != obj->period_ticks) {
        float duty_ratio;

        /* Preserve the duty ratio */
        if (0 == obj->period_ticks)
            duty_ratio = 0;
        else
            duty_ratio = (float)obj->pulsewidth_ticks / (float)obj->period_ticks;
        obj->period_ticks = ticks;
        obj->pulsewidth_ticks = (uint32_t)(ticks * duty_ratio);
        MBED_ASSERT(obj->period_ticks >= obj->pulsewidth_ticks);

        pwmout_update_cfgreg(obj);
    }

    /* Start PWM module */
    pwmout_start(obj);
}

void pwmout_pulsewidth(pwmout_t* obj, float seconds)
{
    pwmout_pulsewidth_us(obj, seconds * 1000000.0f);
}

void pwmout_pulsewidth_ms(pwmout_t* obj, int ms)
{
    pwmout_pulsewidth_us(obj, ms * 1000);
}

/* Set the PWM pulsewidth, keeping the period the same. */
void pwmout_pulsewidth_us(pwmout_t* obj, int us)
{
    uint32_t ticks;

    MBED_ASSERT(PWM_7 >= (PWMName)(obj->channel));

    /* Check if already started */
    if(is_pwmout_started(obj))
        pwmout_stop(obj);

    /* Calculate number of ticks */
    ticks = (uint64_t)obj->base_clk * us / 1000000;

    if(ticks != obj->pulsewidth_ticks) {
        obj->pulsewidth_ticks = ticks;
        MBED_ASSERT(obj->period_ticks >= obj->pulsewidth_ticks);

        pwmout_update_cfgreg(obj);
    }

    /* Start PWM module */
    pwmout_start(obj);
}

static uint8_t is_pwmout_started(pwmout_t* obj)
{
    uint8_t retVal = 0;
    uint32_t reg_val;

    MBED_ASSERT(PWM_7 >= (PWMName)(obj->channel));

    if(PWM_3 >= (PWMName)obj->channel) {
        reg_val = (EXIF_PWM_EN_REG >> 8) & 0x0FUL;
        if(reg_val & (0x01UL << obj->channel))
            retVal = 1;
    } else if(PWM_4 == (PWMName)obj->channel) {
        if(*(obj->CFGR) & (0x01UL << 1))
            retVal = 1;
    } else if(PWM_5 == (PWMName)obj->channel) {
        retVal = 1;
    } else {
        if(*(obj->CFGR) & (0x01UL << 16))
            retVal = 1;
    }

    return retVal;
}

static void pwmout_start(pwmout_t* obj)
{
    MBED_ASSERT(PWM_7 >= (PWMName)(obj->channel));

    if (obj->period_ticks == obj->pulsewidth_ticks) {
        gpio_t gpio;
        gpio_init_out(&gpio, obj->pin);
        gpio_write(&gpio, 1);
        //mbed_error_printf("100\n");
    } else if (0 == obj->pulsewidth_ticks) {
        gpio_t gpio;
        gpio_init_out(&gpio, obj->pin);
        gpio_write(&gpio, 0);
        //mbed_error_printf("0\n");
    } else {
        pinmap_pinout(obj->pin, PinMap_PWM);
    }

    if(PWM_3 >= (PWMName)obj->channel) {
        EXIF_PWM_EN_REG |= (0x01UL << (8 + obj->channel));
    } else if(PWM_4 == (PWMName)obj->channel) {
        *(obj->CFGR) |= 0x01UL;
    } else if(PWM_5 == (PWMName)obj->channel) {
        /* Nothing to be done */
    } else {
        *(obj->CFGR) |= (0x01UL << 16);
    }
}

static void pwmout_stop(pwmout_t* obj)
{
    MBED_ASSERT(PWM_7 >= (PWMName)(obj->channel));

    if (obj->period_ticks == obj->pulsewidth_ticks) {
        gpio_t gpio;
        gpio_init_out(&gpio, obj->pin);
        gpio_write(&gpio, 0);
    }

    if(PWM_3 >= (PWMName)obj->channel) {
        EXIF_PWM_EN_REG &= ~(0x01UL << (8 + obj->channel));
    } else if(PWM_4 == (PWMName)(obj->channel)) {
        *(obj->CFGR) &= ~0x01UL;
    } else if(PWM_5 == (PWMName)(obj->channel)) {
        /* Nothing to be done */
    } else {
        *(obj->CFGR) &= ~(0x01UL << 16);
    }
}

static void pwmout_update_cfgreg(pwmout_t* obj)
{
    if(PWM_3 >= (PWMName)(obj->channel)) {
        if (obj->period_ticks == obj->pulsewidth_ticks) {
            *(obj->CFGR) = ((obj->pulsewidth_ticks - 1) << 16);
        } else {
            *(obj->CFGR) = ((obj->period_ticks - obj->pulsewidth_ticks - 1) & 0xFFFFUL) |
                ((obj->pulsewidth_ticks - 1) << 16);
        }
    } else if(PWM_4 == (PWMName)(obj->channel)) {
        if (obj->pulsewidth_ticks < 8)
            obj->pulsewidth_ticks = 8;
        MBED_ASSERT(((obj->period_ticks >> 1) >= obj->pulsewidth_ticks) &&
            (obj->pulsewidth_ticks >= 8));
        *(obj->CFGR) = ((obj->pulsewidth_ticks & 0x3FFUL) << 4) | ((obj->period_ticks & 0x7FF) << 16);
    } else if(PWM_5 == (PWMName)(obj->channel)) {
        /* TBD */
        uint32_t reg_val = *(obj->CFGR) & ~(0xFUL << 4) & ~(0x7UL << 16);
        uint32_t lpg_field_ontime = (0x01UL << 4) & (0xFUL << 4); // to be confirm
        uint32_t lpg_field_period = (obj->period_ticks << 4) & (0x7UL << 16);
        *(obj->CFGR) = reg_val | lpg_field_ontime | lpg_field_period;
    } else {
    }
}
#endif

