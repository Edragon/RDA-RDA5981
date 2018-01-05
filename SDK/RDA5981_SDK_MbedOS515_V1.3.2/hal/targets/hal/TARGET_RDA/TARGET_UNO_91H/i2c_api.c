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
#if DEVICE_I2C

#include "mbed_assert.h"
#include "i2c_api.h"
#include "cmsis.h"
#include "pinmap.h"

#define I2C_CLKGATE_REG1        (RDA_SCU->CLKGATE1)

#define I2C_CLOCK_SOURCE        (AHBBusClock >> 1)

static const PinMap PinMap_I2C_SDA[] = {
    {PB_2, I2C_0, 1},
    {PD_0, I2C_0, 3},
    {NC  , NC   , 0}
};

static const PinMap PinMap_I2C_SCL[] = {
    {PB_3, I2C_0, 1},
    {PD_1, I2C_0, 3},
    {NC  , NC,    0}
};

static inline void i2c_cmd_cfg(i2c_t *obj, int start, int stop,
                               int acknowledge, int write, int read)
{
    obj->i2c->CMD = (start << 16) | (stop << 8) | (acknowledge << 0) |
                    (write << 12) | (read << 4);
}

static inline void i2c_do_write(i2c_t *obj, int value)
{
    /* write data */
    obj->i2c->DR = value;
}

static inline void i2c_do_read(i2c_t *obj)
{
    uint32_t reg_val;

    /* config read number */
    reg_val = obj->i2c->CR0 & ~(0x1FUL << 9);
    obj->i2c->CR0 = reg_val | (0x01UL << 9);
}

static inline void i2c_clear_fifo(i2c_t *obj) {
    uint32_t reg_val;

    /* clear read number */
    reg_val = obj->i2c->CR0 & ~(0x1FUL << 9);
    obj->i2c->CR0 = reg_val;

    /* clear fifo */
    reg_val = obj->i2c->CR0 | (0x01UL << 7);
    obj->i2c->CR0 = reg_val;

    reg_val = (obj->i2c->CR0) & ~(0x01UL << 7);
    obj->i2c->CR0 = reg_val;
}

void i2c_init(i2c_t *obj, PinName sda, PinName scl)
{
    /* Determine the I2C to use */
    I2CName i2c_sda = (I2CName)pinmap_peripheral(sda, PinMap_I2C_SDA);
    I2CName i2c_scl = (I2CName)pinmap_peripheral(scl, PinMap_I2C_SCL);
    obj->i2c = (RDA_I2C0_TypeDef *)pinmap_merge(i2c_sda, i2c_scl);
    MBED_ASSERT((int)obj->i2c != NC);

    /* Enable I2C clock */
    I2C_CLKGATE_REG1 |= (0x01UL << 6);

    /* set default frequency at 100k */
    i2c_frequency(obj, 100000);
    i2c_cmd_cfg(obj, 0, 0, 0, 0, 0);

    /* Enable I2C */
    obj->i2c->CR0 |= 0x01UL;

    pinmap_pinout(sda, PinMap_I2C_SDA);
    pinmap_pinout(scl, PinMap_I2C_SCL);
}

void i2c_frequency(i2c_t *obj, int hz)
{
    /* Set I2C frequency */
    uint32_t prescale = I2C_CLOCK_SOURCE / ((uint32_t)hz * 5U) - 1U;
    uint32_t reg_val;

    reg_val = obj->i2c->CR0 & ~(0xFFFFUL << 16);
    obj->i2c->CR0 = reg_val | (prescale << 16);
}

int i2c_start(i2c_t *obj)
{
    i2c_cmd_cfg(obj, 1, 0, 0, 0, 0);
    return 0;
}

int i2c_stop(i2c_t *obj)
{
    i2c_cmd_cfg(obj, 0, 1, 0, 0, 0);
    return 0;
}

int i2c_read(i2c_t *obj, int address, char *data, int length, int stop)
{
    int count;

    /* wait tx fifo empty */
    while (((obj->i2c->SR & (0x1FUL << 26)) >> 26) != 0x1F) {
        /* timeout */
        if (obj->i2c->SR & (0x01UL << 31)) {
            i2c_clear_fifo(obj);
            return 0;
        }
    }

    i2c_do_write(obj, address);
    i2c_cmd_cfg(obj, 1, 0, 0, 1, 0);

    /* wait tx fifo empty */
    while (((obj->i2c->SR & (0x1FUL << 26)) >> 26) != 0x1F) {
        if (obj->i2c->SR & (0x01UL << 31)) {
            i2c_clear_fifo(obj);
            return 0;
        }
    }

    /* Read in all except last byte */
    for (count = 0; count < (length - 1); count++) {
        i2c_do_read(obj);
        i2c_cmd_cfg(obj, 0, 0, 0, 0, 1);
        
        /* wait data */
        while ((obj->i2c->SR & (0x1F << 21)) == 0) {
            if (obj->i2c->SR & (0x01UL << 31)) {
                i2c_clear_fifo(obj);
                return count;
            }
        }
        data[count] = (char) (obj->i2c->DR & 0xFF);
    }


    if (stop) {
        i2c_do_read(obj);
        i2c_cmd_cfg(obj, 0, 1, 1, 0, 1);

        /* wait for i2c idle */
        while (obj->i2c->SR & (0x01UL << 16)) {
            if (obj->i2c->SR & (0x01UL << 31)) {
                i2c_clear_fifo(obj);
                return (length - 1);
            }
        }

        if (obj->i2c->SR & (0x1F << 21)) {
            data[count] = (char) (obj->i2c->DR & 0xFF);
            i2c_clear_fifo(obj);
        } else {
            i2c_clear_fifo(obj);
            return (length - 1);
        }
    }

    return length;
}

int i2c_write(i2c_t *obj, int address, const char *data, int length, int stop)
{
    int count;

    /* wait tx fifo empty */
    while (((obj->i2c->SR & (0x1FUL << 26)) >> 26) != 0x1F) {
        if (obj->i2c->SR & (0x01UL << 31)) {
            i2c_clear_fifo(obj);
            return 0;
        }
    }

    i2c_do_write(obj, address);
    i2c_cmd_cfg(obj, 1, 0, 0, 1, 0);

    for (count = 0; count < length - 1; count++) {
        /* wait tx fifo empty */
        while (((obj->i2c->SR & (0x1FUL << 26)) >> 26) != 0x1F) {
            if (obj->i2c->SR & (0x01UL << 31)) {
                i2c_clear_fifo(obj);
                return (count > 0 ? (count - 1) : 0);
            }
        }
        i2c_do_write(obj, data[count]);
        i2c_cmd_cfg(obj, 0, 0, 0, 1, 0);
    }

    /* wait tx fifo empty */
    while (((obj->i2c->SR & (0x1FUL << 26)) >> 26) != 0x1F) {
        if (obj->i2c->SR & (0x01UL << 31)) {
            i2c_clear_fifo(obj);
            return (count > 0 ? (count - 1) : 0);
        }
    }

    i2c_do_write(obj, data[length - 1]);
    if (stop) {
        i2c_cmd_cfg(obj, 0, 1, 0, 1, 0);
    } else {
        i2c_cmd_cfg(obj, 0, 0, 0, 1, 0);
    }

    /* wait tx fifo empty */
    while (((obj->i2c->SR & (0x1FUL << 26)) >> 26) != 0x1F) {
        if (obj->i2c->SR & (0x01UL << 31)) {
            i2c_clear_fifo(obj);
            return (length - 1);
        }
    }

    if (stop) {
        i2c_clear_fifo(obj);
    }

    return length;
}

void i2c_reset(i2c_t *obj)
{
    i2c_cmd_cfg(obj, 0, 1, 0, 0, 0);
}

int i2c_byte_read(i2c_t *obj, int last)
{
    int value;

    i2c_do_read(obj);
    if (last) {
        i2c_cmd_cfg(obj, 0, 1, 1, 0, 1);
        while (obj->i2c->SR & (0x01UL << 16)) {
            if (obj->i2c->SR & (0x01UL << 31)) {
                i2c_clear_fifo(obj);
                return 0xFF;
            }
        }
        if (obj->i2c->SR & (0x1F << 21)) {
            value = obj->i2c->DR & 0xFF;
            i2c_clear_fifo(obj);
        } else {
            i2c_clear_fifo(obj);
            return 0xFF;
        }
    } else {
        i2c_cmd_cfg(obj, 0, 0, 0, 0, 1);
        while ((obj->i2c->SR & (0x1F << 21)) == 0) {
            if (obj->i2c->SR & (0x01UL << 31)) {
                i2c_clear_fifo(obj);
                return 0xFF;
            }
        }
        value = obj->i2c->DR & 0xFF;
    }

    return value;
}

int i2c_byte_write(i2c_t *obj, int data)
{
    int ack = 1;

    i2c_do_write(obj, (data & 0xFF));
    i2c_cmd_cfg(obj, 0, 0, 0, 1, 0);
    while (((obj->i2c->SR & (0x1FUL << 26)) >> 26) != 0x1F) {
        if (obj->i2c->SR & (0x01UL << 31)) {
            i2c_clear_fifo(obj);
            ack = 0;
        }
    }

    return ack;
}

#endif

