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
#include <string.h>
#include "rda_i2s_api.h"
#include "PeripheralNames.h"
#include "pinmap.h"
#include "gpio_api.h"

/*------------- Wlan Monitor (WLANMON) ---------------------------------------*/
typedef struct
{
  __IO uint32_t PHYSELGRP[4];           /* 0x00-0x0C : PHY select group 0 - 3 */
} RDA_WLANMON_TypeDef;

/** Macros
 */
#define RDA_MON                 ((RDA_WLANMON_TypeDef *)RDA_MON_BASE)

/** I2S HAL macro
 */
#define DFLT_TXFIFO_ALMOSTEMPTY_WDLEN   (31)
#define DFLT_RXFIFO_ALMOSTFULL_WDLEN    (12)

/* Misc status reg */
#define I2S_TXFIFO_ALMOSTEMPTY_CLR_BIT  (0x01UL)
#define I2S_RXFIFO_ALMOSTFULL_CLR_BIT   (0x01UL << 1)
#define I2S_TXFIFO_ALMOSTEMPTY_ENBIT    (0x01UL << 2)
#define I2S_TXFIFO_WRITEWHENFULL_ENBIT  (0x01UL << 3)
#define I2S_TXFIFO_READWHENEMPTY_ENBIT  (0x01UL << 4)
#define I2S_TXFIFO_FULL_ENBIT           (0x01UL << 5)
#define I2S_RXFIFO_ALMOSTFULL_ENBIT     (0x01UL << 6)
#define I2S_RXFIFO_WRITEWHENFULL_ENBIT  (0x01UL << 7)
#define I2S_TXFIFO_ENBITS               (0x0FUL << 2)
#define I2S_RXFIFO_ENBITS               (0x03UL << 6)
#define I2S_TXFIFO_ALMOSTEMPTY_BIT      (0x01UL << 18)
#define I2S_TXFIFO_WRITEWHENFULL_BIT    (0x01UL << 19)
#define I2S_TXFIFO_READWHENEMPTY_BIT    (0x01UL << 20)
#define I2S_TXFIFO_FULL_BIT             (0x01UL << 21)
#define I2S_RXFIFO_ALMOSTFULL_BIT       (0x01UL << 22)
#define I2S_RXFIFO_WRITEWHENFULL_BIT    (0x01UL << 23)
#define I2S_TXFIFO_BITS                 (0x0FUL << 18)
#define I2S_RXFIFO_BITS                 (0x03UL << 22)

/* Misc status reg */
#define I2S_TXINT_ENBIT                 (0x01UL << 11)
#define I2S_RXINT_ENBIT                 (0x01UL << 12)

#define I2S_MASTER_RX_IOMUX             {I2S_TX_BCLK, I2S_TX_WS, I2S_RX_SD}
#define I2S_MASTER_TX_IOMUX             {I2S_TX_BCLK, I2S_TX_WS, I2S_TX_SD}
#define I2S_SLAVE_RX_IOMUX              {I2S_RX_BCLK, I2S_RX_WS, I2S_RX_SD}

/** I2S Reg list
 */
#define I2S_CLKGATE_REG1            (RDA_SCU->CLKGATE1)
#define I2S_CLKGATE_REG2            (RDA_SCU->CLKGATE2)
#define I2S_CLK_DIV_REG             (RDA_SCU->I2SCLKDIV)
#define EXIF_MISC_STCFG_REG         (RDA_EXIF->MISCSTCFG)
#define EXIF_MISC_INTCFG_REG        (RDA_EXIF->MISCINTCFG)
#define EXIF_MISC_CFG_REG           (RDA_EXIF->MISCCFG)
#define MEM_CFG_REG                 (RDA_GPIO->MEMCFG)

static const PinMap PinMap_I2S_TX_BCLK[] = {
    {PB_3,  I2S_0,  3},
    {NC  ,  NC   ,  0}
};

static const PinMap PinMap_I2S_TX_WS[] = {
    {PB_2,  I2S_0,  3},
    {NC  ,  NC   ,  0}
};

static const PinMap PinMap_I2S_TX_SD[] = {
    {PB_1,  I2S_0,  3},
    {NC  ,  NC   ,  0}
};

static const PinMap PinMap_I2S_RX_BCLK[] = {
    {PB_8,  I2S_0,  3},
    {NC  ,  NC   ,  0}
};

static const PinMap PinMap_I2S_RX_WS[] = {
    {PB_5,  I2S_0,  3},
    {NC  ,  NC   ,  0}
};

static const PinMap PinMap_I2S_RX_SD[] = {
    {PB_4,  I2S_0,  3},
    {NC  ,  NC   ,  0}
};

static const PinMap PinMap_I2S_MCLK[] = {
    {PB_0,  I2S_0,  2},
    {PB_1,  I2S_0,  2},
    {PB_2,  I2S_0,  2},
    {PB_3,  I2S_0,  2},
    {PB_4,  I2S_0,  2},
    {PB_5,  I2S_0,  2},
    {PB_6,  I2S_0,  2},
    {PB_7,  I2S_0,  2},
    {PB_8,  I2S_0,  2},
    {PB_9,  I2S_0,  2},
    {PC_0,  I2S_0,  2},
    {PC_1,  I2S_0,  2},
    {NC  ,  NC   ,  0}
};

static i2s_t *rda_i2s_obj;

static osSemaphoreId tx_sem_id;
static osSemaphoreId rx_sem_id;
static osSemaphoreDef_t tx_sem_def;
static osSemaphoreDef_t rx_sem_def;
static uint32_t tx_sem_data[2];
static uint32_t rx_sem_data[2];
static BOOL tx_waiting;
static BOOL rx_waiting;
BOOL i2s_tx_enabled;
BOOL i2s_rx_enabled;
const RDA_I2S_SEM_T i2s_tx_sem = TX_SEM;
const RDA_I2S_SEM_T i2s_rx_sem = RX_SEM;

static inline void block_transmit(i2s_t *obj);
static inline void block_receive(i2s_t *obj);
static inline void enable_i2s_tx_int(void);
static inline void disable_i2s_tx_int(void);
static inline void enable_i2s_rx_int(void);
static inline void disable_i2s_rx_int(void);
static inline void clear_i2s_tx_int(void);
static inline void clear_i2s_rx_int(void);

extern void rda_exif_irq_set(void);

void rda_i2s_init(i2s_t *obj, PinName tx_bclk, PinName tx_ws, PinName tx_sd,
                              PinName rx_bclk, PinName rx_ws, PinName rx_sd, PinName mclk)
{
    uint32_t reg_val = 0x00;
    I2SName i2s_bclk, i2s_ws, i2s_sd, i2s_tx_sel, i2s_rx_sel, i2s_mclk;

    /* Determine the I2S to use */
    i2s_bclk = (I2SName)pinmap_peripheral(tx_bclk, PinMap_I2S_TX_BCLK);
    i2s_ws   = (I2SName)pinmap_peripheral(tx_ws, PinMap_I2S_TX_WS);
    i2s_sd   = (I2SName)pinmap_peripheral(tx_sd, PinMap_I2S_TX_SD);

    i2s_tx_sel = (I2SName)pinmap_merge(i2s_bclk, i2s_ws);
    i2s_tx_sel = (I2SName)pinmap_merge(i2s_tx_sel, i2s_sd);

    i2s_bclk = (I2SName)pinmap_peripheral(rx_bclk, PinMap_I2S_RX_BCLK);
    i2s_ws   = (I2SName)pinmap_peripheral(rx_ws, PinMap_I2S_RX_WS);
    i2s_sd   = (I2SName)pinmap_peripheral(rx_sd, PinMap_I2S_RX_SD);

    i2s_rx_sel = (I2SName)pinmap_merge(i2s_bclk, i2s_ws);
    i2s_rx_sel = (I2SName)pinmap_merge(i2s_rx_sel, i2s_sd);

    i2s_mclk = (I2SName)pinmap_peripheral(mclk, PinMap_I2S_MCLK);

    obj->hw.i2s  = (RDA_I2S_TypeDef *)pinmap_merge(i2s_tx_sel, i2s_rx_sel);
    MBED_ASSERT((I2SName)NC != (I2SName)obj->hw.i2s);

    if((I2SName)NC != i2s_tx_sel) {
        pinmap_pinout(tx_bclk, PinMap_I2S_TX_BCLK);
        pinmap_pinout(tx_ws, PinMap_I2S_TX_WS);
        pinmap_pinout(tx_sd, PinMap_I2S_TX_SD);
        I2S_CLKGATE_REG2 |= (0x01UL << 17);
    }

    if((I2SName)NC != i2s_rx_sel) {
        pinmap_pinout(rx_bclk, PinMap_I2S_RX_BCLK);
        pinmap_pinout(rx_ws, PinMap_I2S_RX_WS);
        pinmap_pinout(rx_sd, PinMap_I2S_RX_SD);
        I2S_CLKGATE_REG2 |= (0x01UL << 16);
    }

    /* Make sure SPI MOSI pin is not PB_3 when using I2S MCLK pin */
    if((I2SName)NC != i2s_mclk) {
        int port, idx = 0, ofst = 0;
        pinmap_pinout(mclk, PinMap_I2S_MCLK);
        reg_val = RDA_GPIO->CTRL & ~(0x0FUL /*<< 0*/);
        RDA_GPIO->CTRL = reg_val |  (0x06UL /*<< 0*/);
        reg_val = RDA_SCU->CLKGATE3 & ~(0x0FUL << 8);
        RDA_SCU->CLKGATE3 = reg_val |  (0x02UL << 8);
        port =  ((int)mclk) >> PORT_SHIFT;
        idx  = (((int)mclk) >> 2) & 0x0003;
        ofst = (((int)mclk) & 0x0003) << 3;
        if(4 == port) {
            idx += 3;
        }
        reg_val = RDA_MON->PHYSELGRP[idx] & ~(0x3FUL << ofst);
        RDA_MON->PHYSELGRP[idx] = reg_val |  (0x02UL << ofst);
    }

    /* Enable I2S clock */
    I2S_CLKGATE_REG1 |= (0x01UL << 1);

    /* Set exif interrupt irq handler */
    rda_exif_irq_set();

    /* Config I2S interrupt status config reg */
    reg_val = EXIF_MISC_STCFG_REG | I2S_TXFIFO_ALMOSTEMPTY_ENBIT | I2S_RXFIFO_ENBITS;
    EXIF_MISC_STCFG_REG = reg_val;

    /* Auto-clear almost-empty flag, hw autogen data[31], start from left channel */
    reg_val  = EXIF_MISC_CFG_REG | (0x01UL << 5) | (0x01UL << 17);
    reg_val &= ~(0x01UL << 18);
    EXIF_MISC_CFG_REG = reg_val;

    obj->sw_tx.state = I2S_ST_RESET;
    obj->sw_rx.state = I2S_ST_RESET;

    /* Store i2s object to global variable */
    rda_i2s_obj = obj;

    /* Create I2S tx and rx semaphore */
    rda_i2s_sem_create(TX_SEM, 0);
    rda_i2s_sem_create(RX_SEM, 0);
}

void rda_i2s_deinit(i2s_t *obj)
{
    /* Disable I2S TX/RX */
    obj->hw.i2s->CFG &= ~(0x01UL | (0x01UL << 16));

    /* Disable I2S clock */
    I2S_CLKGATE_REG1 &= ~(0x01UL << 1);

    I2S_CLKGATE_REG2 &= ~((0x01L << 17) | (0x01L << 16));

    /* Disable I2S interrupt */
    EXIF_MISC_INTCFG_REG &= ~(I2S_TXINT_ENBIT | I2S_RXINT_ENBIT);

    /* Clear I2S RX interrupt */
    if (EXIF_MISC_STCFG_REG & I2S_RXFIFO_ALMOSTFULL_BIT) {
        EXIF_MISC_STCFG_REG |= I2S_RXFIFO_ALMOSTFULL_CLR_BIT;
        EXIF_MISC_STCFG_REG &= ~(I2S_RXFIFO_ALMOSTFULL_BIT | I2S_RXFIFO_ALMOSTFULL_CLR_BIT);
    }

    /* Clear I2S TX interrupt */
    if (EXIF_MISC_STCFG_REG & I2S_TXFIFO_ALMOSTEMPTY_BIT) {
        EXIF_MISC_STCFG_REG &= ~I2S_TXFIFO_ALMOSTEMPTY_BIT;
    }

    /* Delete I2S tx and rx semaphore */
    rda_i2s_sem_delete(TX_SEM);
    rda_i2s_sem_delete(RX_SEM);
}

void rda_i2s_format(i2s_t *obj, i2s_cfg_t *cfg)
{
    uint32_t reg_val = 0x00;

    if (I2S_MD_MASTER_RX == cfg->mode) {
        /*i2s_clk/ws out, i2s_data in */
        MEM_CFG_REG |= (0x01UL << 10);

        /* Re-config I2S mode to init hw */
        cfg->mode |= 0x03;
    }

    if (I2S_MD_MASTER_TX & cfg->mode) {
        /* Enable I2S TX clock */
        I2S_CLKGATE_REG2 |= (0x01UL << 17);

        /* Get I2S params and calculate I2S cfg reg value*/
        if((uint8_t)I2S_32FS == cfg->tx.fs)
            reg_val |= (0x01UL << 1);

        if((uint8_t)I2S_WS_POS == cfg->tx.ws_polarity)
            reg_val |= (0x01UL << 2);

        if((uint8_t)I2S_STD_M == cfg->tx.std_mode)
            reg_val |= (0x01UL << 3);

        if((uint8_t)I2S_LEFT_JM == cfg->tx.justified_mode)
            reg_val |= (0x01UL << 4);

        if((uint8_t)I2S_DL_20b == cfg->tx.data_len)
            reg_val |= (0x01UL << 5);
        else if((uint8_t)I2S_DL_24b == cfg->tx.data_len)
            reg_val |= (0x02UL << 5);

        if((uint8_t)I2S_MSB == cfg->tx.msb_lsb)
            reg_val |= (0x01UL << 7);

        if((uint8_t)I2S_WF_CNTLFT_2W == cfg->tx.wrfifo_cntleft)
            reg_val |= (0x01UL << 9);
        else if((uint8_t)I2S_WF_CNTLFT_4W == cfg->tx.wrfifo_cntleft)
            reg_val |= (0x02UL << 9);
        else if((uint8_t)I2S_WF_CNTLFT_8W == cfg->tx.wrfifo_cntleft)
            reg_val |= (0x03UL << 9);

        obj->sw_tx.regBitOfst = 16 - (cfg->tx.data_len << 2);

        switch (cfg->tx.data_len) {
        case I2S_DL_8b:
            obj->sw_tx.dataWidth = 1;
            break;
        case I2S_DL_12b:
        case I2S_DL_16b:
            obj->sw_tx.dataWidth = 2;
            break;
        case I2S_DL_20b:
        case I2S_DL_24b:
            obj->sw_tx.dataWidth = 4;
            break;
        default:
            obj->sw_tx.dataWidth = 2;
            break;
        }
        obj->sw_tx.fifoThreshold = 32 - (0x01 << cfg->tx.wrfifo_cntleft);
        obj->sw_tx.state = I2S_ST_READY;
    }

    if (I2S_MD_SLAVE_RX & cfg->mode) {
        /* Enable I2S RX clock */
        I2S_CLKGATE_REG2 |= (0x01UL << 16);

        /* Get I2S params and calculate I2S cfg reg value*/
        if((uint8_t)I2S_32FS == cfg->rx.fs)
            reg_val |= (0x01UL << 17);

        if((uint8_t)I2S_WS_POS == cfg->rx.ws_polarity)
            reg_val |= (0x01UL << 18);

        if((uint8_t)I2S_STD_M == cfg->rx.std_mode)
            reg_val |= (0x01UL << 19);

        if((uint8_t)I2S_LEFT_JM == cfg->rx.justified_mode)
            reg_val |= (0x01UL << 20);

        if((uint8_t)I2S_DL_20b == cfg->rx.data_len)
            reg_val |= (0x01UL << 21);
        else if((uint8_t)I2S_DL_24b == cfg->rx.data_len)
            reg_val |= (0x02UL << 21);

        if((uint8_t)I2S_MSB == cfg->rx.msb_lsb)
            reg_val |= (0x01UL << 23);

        obj->sw_rx.regBitOfst = 16 - (cfg->rx.data_len << 2);

        switch (cfg->rx.data_len) {
        case I2S_DL_8b:
            obj->sw_rx.dataWidth = 1;
            break;
        case I2S_DL_12b:
        case I2S_DL_16b:
            obj->sw_rx.dataWidth = 2;
            break;
        case I2S_DL_20b:
        case I2S_DL_24b:
            obj->sw_rx.dataWidth = 4;
            break;
        default:
            obj->sw_rx.dataWidth = 2;
            break;
        }
        obj->sw_rx.fifoThreshold = DFLT_RXFIFO_ALMOSTFULL_WDLEN;
        obj->sw_rx.state = I2S_ST_READY;
    }

    /* Config I2S cfg reg */
    obj->hw.i2s->CFG = reg_val;
}

void rda_i2s_enable_tx(i2s_t *obj)
{
    uint32_t reg_val = obj->hw.i2s->CFG;
    reg_val |= 0x01UL;
    obj->hw.i2s->CFG = reg_val;
    i2s_tx_enabled = TRUE;
}

void rda_i2s_disable_tx(i2s_t *obj)
{
    uint32_t reg_val = obj->hw.i2s->CFG;
    reg_val &= ~0x01UL;
    obj->hw.i2s->CFG = reg_val;
    i2s_tx_enabled = FALSE;
    disable_i2s_tx_int();
    clear_i2s_tx_int();
}

void rda_i2s_enable_rx(i2s_t *obj)
{
    uint32_t reg_val = obj->hw.i2s->CFG;
    reg_val |= (0x01UL << 16);
    obj->hw.i2s->CFG = reg_val;
    i2s_rx_enabled = TRUE;
}

void rda_i2s_disable_rx(i2s_t *obj)
{
    uint32_t reg_val = obj->hw.i2s->CFG;
    reg_val &= ~(0x01UL << 16);
    obj->hw.i2s->CFG = reg_val;
    i2s_rx_enabled = FALSE;
    disable_i2s_rx_int();
    clear_i2s_rx_int();
}

void rda_i2s_enable_master_rx(void)
{
    /* i2s_clk/ws out, i2s_data in */
    MEM_CFG_REG |= (0x01UL << 10);
}

void rda_i2s_disable_master_rx(void)
{
    /* i2s_clk/ws/data all in(out) */
    MEM_CFG_REG &= ~(0x01UL << 10);
}

void rda_i2s_out_mute(i2s_t *obj)
{
    __O uint32_t *rTxFifo = &(obj->hw.i2s->DOUTWR);
    *rTxFifo = 0;
    *rTxFifo = 0;
}

uint8_t rda_i2s_set_channel(i2s_t *obj, uint8_t channel)
{
    if ((channel > 2) && (channel < 1)) {
        return 1;
    }

    obj->sw_tx.channels = channel;
    obj->sw_rx.channels = channel;

    return 0;
}

uint8_t rda_i2s_set_tx_channel(i2s_t *obj, uint8_t channel)
{
    if ((channel > 2) && (channel < 1)) {
        return 1;
    }

    obj->sw_tx.channels = channel;

    return 0;
}

uint8_t rda_i2s_set_rx_channel(i2s_t *obj, uint8_t channel)
{
    if ((channel > 2) && (channel < 1)) {
        return 1;
    }

    obj->sw_rx.channels = channel;

    return 0;
}

uint8_t rda_i2s_set_rx_mono_channel(i2s_t *obj, uint8_t channel)
{
    if (channel > 1) {
        return 1;
    }

    obj->sw_rx.monoChannel = channel;

    return 0;
}

uint8_t rda_i2s_set_ws(i2s_t *obj, uint32_t ws, uint32_t ratio)
{
    uint32_t bclk;
    uint32_t int_div1 = 2;
    uint32_t int_div0;
    uint32_t frac_div0;
    uint32_t mclk;
    uint8_t  fs;// ws/bclk

    fs = (obj->hw.i2s->CFG & (0x01UL << 1)) ? 32 : 64;

    if ((ws *ratio) > 40000000)
        return 1;

    if (32 == fs) {//32FS
        if (ratio < 64)
            return 2;
    } else {//64FS
        if (ratio < 128)
            return 2;
    }

    bclk = ws * fs;

    int_div1 = ratio / fs - 2;

    mclk = bclk * (int_div1 + 2);

    int_div0 = 160000000 / mclk;

    float tmp = 160000000 / (float) mclk;

    frac_div0 = (tmp - int_div0) * 65535;

    I2S_CLK_DIV_REG = (int_div1 << 24) | (int_div0 << 16) | (frac_div0 & 0xFFFF);

    return 0;
}

uint8_t rda_i2s_blk_send(i2s_t *obj, uint32_t *buf, uint16_t size)
{
    if((NULL == buf) || (0 == size))
        return 1;   /* Error args */

    if(I2S_ST_READY == obj->sw_tx.state) {
        obj->sw_tx.state = I2S_ST_BUSY;
        obj->sw_tx.buffer = buf;
        obj->sw_tx.bufferSize = size;
        obj->sw_tx.bufferCntr = 0;

        block_transmit(obj);

        obj->sw_tx.state = I2S_ST_READY;

        return 0;   /* Send done */
    } else {
        return 2;   /* Busy */
    }
}

uint8_t rda_i2s_blk_recv(i2s_t *obj, uint32_t *buf, uint16_t size)
{
    if((NULL == buf) || (0 == size))
        return 1;   /* Error args */

    if(I2S_ST_READY == obj->sw_rx.state) {
        obj->sw_rx.state = I2S_ST_BUSY;
        obj->sw_rx.buffer = buf;
        obj->sw_rx.bufferSize = size;
        obj->sw_rx.bufferCntr = 0;

        block_receive(obj);

        obj->sw_rx.state = I2S_ST_READY;

        return 0;   /* Recv done */
    } else {
        return 2;   /* Busy */
    }
}

uint8_t rda_i2s_int_send(i2s_t *obj, uint32_t *buf, uint16_t size)
{
    if((NULL == buf) || (0 == size))
        return 1;   /* Error args */

    if(I2S_ST_READY == obj->sw_tx.state) {
        obj->sw_tx.state = I2S_ST_BUSY;
        obj->sw_tx.buffer = buf;
        obj->sw_tx.bufferSize = size;
        obj->sw_tx.bufferCntr = 0;

        if(obj->sw_tx.bufferSize > (obj->sw_tx.fifoThreshold * obj->sw_tx.dataWidth / sizeof(uint32_t) / (obj->sw_tx.channels == 1 ? 2 : 1))) {
            enable_i2s_tx_int();
        } else {
            block_transmit(obj);
            obj->sw_tx.state = I2S_ST_READY;
        }

        return 0;   /* Send done */
    } else {
        return 2;   /* Busy */
    }
}


uint8_t rda_i2s_int_recv(i2s_t *obj, uint32_t *buf, uint16_t size)
{
    if((NULL == buf) || (0 == size))
        return 1;   /* Error args */

    if(I2S_ST_READY == obj->sw_rx.state) {
        obj->sw_rx.state = I2S_ST_BUSY;
        obj->sw_rx.buffer = buf;
        obj->sw_rx.bufferSize = size;
        obj->sw_rx.bufferCntr = 0;

        if(obj->sw_rx.bufferSize > (obj->sw_rx.fifoThreshold * obj->sw_rx.dataWidth / sizeof(uint32_t) / (obj->sw_rx.channels == 1 ? 2 : 1))) {
            enable_i2s_rx_int();
        } else {
            block_receive(obj);
            obj->sw_rx.state = I2S_ST_READY;
        }

        return 0;   /* Recv done */
    } else {
        return 2;   /* Busy */
    }
}

void rda_i2s_irq_handler(uint32_t int_status)
{
    i2s_t *obj = rda_i2s_obj;

    if (int_status & I2S_RXFIFO_WRITEWHENFULL_BIT) {
        __I uint32_t *rRxFifo = &(obj->hw.i2s->DINRD);
        for (uint32_t i = 0; i < 16; i++) {
            uint32_t tmp_val = *rRxFifo;
            tmp_val = tmp_val;
        }
        /* Clear I2S RX interrupt */
        EXIF_MISC_STCFG_REG |= I2S_RXFIFO_ALMOSTFULL_CLR_BIT;
        EXIF_MISC_STCFG_REG &= ~(I2S_RXFIFO_BITS | I2S_RXFIFO_ALMOSTFULL_CLR_BIT);
        int_status &= ~(I2S_RXFIFO_WRITEWHENFULL_BIT | I2S_RXFIFO_ALMOSTFULL_BIT);
    }

    /* Get RX FIFO flag */
    if(int_status & I2S_RXFIFO_ALMOSTFULL_BIT) {
        uint16_t start_cnt = obj->sw_rx.bufferCntr;
        uint16_t end_cnt   = start_cnt + (obj->sw_rx.fifoThreshold * obj->sw_rx.dataWidth / sizeof(uint32_t) / (obj->sw_rx.channels == 1 ? 2 : 1));
        __I uint32_t *rRxFifo = &(obj->hw.i2s->DINRD);
        uint32_t tmp_val = 0;
        uint8_t channel_loop = obj->sw_rx.channels == 1 ? 2 : 1;

        obj->sw_rx.bufferCntr = (end_cnt >= obj->sw_rx.bufferSize) ? obj->sw_rx.bufferSize : end_cnt;

        /* Read RX FIFO register */
        if(1 == obj->sw_rx.dataWidth) {
            /* byte format data buffer */
            while (start_cnt < obj->sw_rx.bufferCntr) {
                tmp_val = 0;
                for (int8_t i = 0; i < 32; i += 8) {
                    for (uint8_t j = channel_loop; j > 0; j--) {
                        uint32_t rx_val = *rRxFifo;
                        /* if channel is mono and input data is not the setted channel */
                        if ((1 == obj->sw_rx.channels) && ((uint32_t)obj->sw_rx.monoChannel ^ (rx_val >> 31))) {
                            continue;
                        }
                        tmp_val |= (((rx_val & 0x00FFFF00) >> obj->sw_rx.regBitOfst) << i);
                    }
                }
                *(obj->sw_rx.buffer + start_cnt) = tmp_val;
                start_cnt++;
            }
        } else if (2 == obj->sw_rx.dataWidth) {
            /* half-word format data buffer, bufferSize is the times of 2, read twice one loop */
            while (start_cnt < obj->sw_rx.bufferCntr) {
                tmp_val = 0;
                for (int8_t i = 0; i < 32; i += 16) {
                    #if 0
                    for (uint8_t j = (obj->sw_rx.channels == 1 ? 2 : 1); j > 0; j--) {
                        tmp_val |= (((*rRxFifo & 0x00FFFF00) >> obj->sw_rx.regBitOfst) << i);
                    }
                    #else
                    for (uint8_t j = channel_loop; j > 0; j--) {
                        uint32_t rx_val = *rRxFifo;
                        /* if channel is mono and input data is not the setted channel */
                        if ((1 == obj->sw_rx.channels) && ((uint32_t)obj->sw_rx.monoChannel ^ (rx_val >> 31))) {
                            continue;
                        }
                        tmp_val |= (((rx_val & 0x00FFFF00) >> obj->sw_rx.regBitOfst) << i);
                    }
                    #endif
                }
                *(obj->sw_rx.buffer + start_cnt) = tmp_val;
                start_cnt++;
            }
        } else if (4 == obj->sw_rx.dataWidth) {
            /* word format data buffer */
            while (start_cnt < obj->sw_rx.bufferCntr) {
                for (uint8_t j = channel_loop; j > 0; j--) {
                        uint32_t rx_val = *rRxFifo;
                        /* if channel is mono and input data is not the setted channel */
                        if ((1 == obj->sw_rx.channels) && ((uint32_t)obj->sw_rx.monoChannel ^ (rx_val >> 31))) {
                            continue;
                        }
                        tmp_val = rx_val & 0x00FFFFFF;
                    }
                *(obj->sw_rx.buffer + start_cnt) = tmp_val;
                start_cnt++;
            }
        }


        if(end_cnt >= obj->sw_rx.bufferSize) {
            disable_i2s_rx_int();
            obj->sw_rx.state = I2S_ST_READY;
            if (rx_waiting) {
                rda_i2s_sem_release(RX_SEM);
                rx_waiting = FALSE;
            }
        }
        /* Clear I2S RX interrupt */
        EXIF_MISC_STCFG_REG |= I2S_RXFIFO_ALMOSTFULL_CLR_BIT;
        EXIF_MISC_STCFG_REG &= ~(I2S_RXFIFO_ALMOSTFULL_BIT | I2S_RXFIFO_ALMOSTFULL_CLR_BIT);
    }

    /* Get TX FIFO flag */
    if(int_status & I2S_TXFIFO_ALMOSTEMPTY_BIT) {
        uint16_t start_cnt = obj->sw_tx.bufferCntr;
        uint16_t end_cnt   = start_cnt + (obj->sw_tx.fifoThreshold * obj->sw_tx.dataWidth / sizeof(uint32_t) / (obj->sw_tx.channels == 1 ? 2 : 1));
        __O uint32_t *rTxFifo = &(obj->hw.i2s->DOUTWR);
        uint32_t tmp_val = 0;

        obj->sw_tx.bufferCntr = (end_cnt >= obj->sw_tx.bufferSize) ? obj->sw_tx.bufferSize : end_cnt;

        if (1 == obj->sw_tx.dataWidth) {
            /* byte format data buffer */
            while (start_cnt < obj->sw_tx.bufferCntr) {
                tmp_val = *(obj->sw_tx.buffer + start_cnt);
                for (uint8_t i = 0; i < 32; i += 8) {
                    for (uint8_t j = (obj->sw_tx.channels == 1 ? 2 : 1); j > 0; j--) {
                        *rTxFifo = (tmp_val >> i) << obj->sw_tx.regBitOfst;
                    }
                }
                start_cnt++;
            }
        } else if (2 == obj->sw_tx.dataWidth) {
            /* half-word format data buffer */
            while (start_cnt < obj->sw_tx.bufferCntr) {
                tmp_val = *(obj->sw_tx.buffer + start_cnt);
                for (uint8_t i = 0; i < 32; i += 16) {
                    for (uint8_t j = (obj->sw_tx.channels == 1 ? 2 : 1); j > 0; j--) {
                        *rTxFifo = (tmp_val >> i) << obj->sw_tx.regBitOfst;
                    }
                }
                start_cnt++;
            }
        } else if (4 == obj->sw_tx.dataWidth) {
            /* word format data buffer */
            while (start_cnt < obj->sw_tx.bufferCntr) {
                tmp_val = *(obj->sw_tx.buffer + start_cnt);
                for (uint8_t j = (obj->sw_tx.channels == 1 ? 2 : 1); j > 0; j--) {
                    *rTxFifo = tmp_val << obj->sw_tx.regBitOfst;
                }
                start_cnt++;
            }
        }

        if(end_cnt >= obj->sw_tx.bufferSize) {
            disable_i2s_tx_int();
            obj->sw_tx.state = I2S_ST_READY;
            if (tx_waiting) {
                rda_i2s_sem_release(TX_SEM);
                tx_waiting = FALSE;
            }
        }
        /* Clear I2S TX interrupt */
        EXIF_MISC_STCFG_REG &= ~I2S_TXFIFO_ALMOSTEMPTY_BIT;
    }
}

static inline void enable_i2s_tx_int(void)
{
    EXIF_MISC_INTCFG_REG |= I2S_TXINT_ENBIT;
}

static inline void disable_i2s_tx_int(void)
{
    EXIF_MISC_INTCFG_REG &= ~I2S_TXINT_ENBIT;
}

static inline void enable_i2s_rx_int(void)
{
    EXIF_MISC_INTCFG_REG |= I2S_RXINT_ENBIT;
}

static inline void disable_i2s_rx_int(void)
{
    EXIF_MISC_INTCFG_REG &= ~I2S_RXINT_ENBIT;
}

static inline void clear_i2s_tx_int(void)
{
    EXIF_MISC_STCFG_REG &= ~I2S_TXFIFO_BITS;
}

static inline void clear_i2s_rx_int(void)
{
    EXIF_MISC_STCFG_REG |= I2S_RXFIFO_ALMOSTFULL_CLR_BIT;
    EXIF_MISC_STCFG_REG &= ~(I2S_RXFIFO_BITS | I2S_RXFIFO_ALMOSTFULL_CLR_BIT);
}

static inline void block_transmit(i2s_t *obj)
{
    while(obj->sw_tx.bufferCntr < obj->sw_tx.bufferSize) {
        __O uint32_t *rTxFifo = &(obj->hw.i2s->DOUTWR);
        uint32_t tmp_val = 0;

        /* Loop forever if TX FIFO is full */
        while(EXIF_MISC_STCFG_REG & I2S_TXFIFO_FULL_BIT);

        /* Write TX FIFO register */
        if (1 == obj->sw_tx.dataWidth) {
            /* byte format data buffer */
            tmp_val = *(obj->sw_tx.buffer + obj->sw_tx.bufferCntr);
            for (int8_t i = 0; i < 32; i += 8) {
                for (uint8_t j = (obj->sw_tx.channels == 1 ? 2 : 1); j > 0; j--) {
                    *rTxFifo = (tmp_val >> i) << obj->sw_tx.regBitOfst;
                }
            }
        } else if (2 == obj->sw_tx.dataWidth) {
            /* half-word format data buffer */
            tmp_val = *(obj->sw_tx.buffer + obj->sw_tx.bufferCntr);
            for (int8_t i = 0; i < 32; i += 16) {
                for (uint8_t j = (obj->sw_tx.channels == 1 ? 2 : 1); j > 0; j--) {
                    *rTxFifo = (tmp_val >> i) << obj->sw_tx.regBitOfst;
                }
            }
        } else if (4 == obj->sw_tx.dataWidth) {
            /* word format data buffer */
            tmp_val = *(obj->sw_tx.buffer + obj->sw_tx.bufferCntr);
            for (uint8_t j = (obj->sw_tx.channels == 1 ? 2 : 1); j > 0; j--) {
                *rTxFifo = tmp_val << obj->sw_tx.regBitOfst;
            }
        }
        obj->sw_tx.bufferCntr++;
    }
}

static inline void block_receive(i2s_t *obj)
{
    while(obj->sw_rx.bufferCntr < obj->sw_rx.bufferSize) {
        __I uint32_t *rRxFifo = &(obj->hw.i2s->DINRD);
        uint32_t tmp_val = 0;

        /* Loop forever if RX FIFO is not empty, check almost-full flag here */
        while(!(EXIF_MISC_STCFG_REG & I2S_RXFIFO_ALMOSTFULL_BIT));

        /* Read RX FIFO register */
        if(1 == obj->sw_rx.dataWidth) {
            /* byte format data buffer */
            for (int8_t i = 0; i < 32; i += 8) {
                for (uint8_t j = (obj->sw_rx.channels == 1 ? 2 : 1); j > 0; j--) {
                    tmp_val |= ((*rRxFifo & 0x00FFFF00) >> obj->sw_rx.regBitOfst) << i;
                }
            }
        } else if (2 == obj->sw_rx.dataWidth) {
            /* half-word format data buffer, bufferSize is the times of 2, read twice one loop */
            for (int8_t i = 0; i < 32; i += 16) {
                for (uint8_t j = (obj->sw_rx.channels == 1 ? 2 : 1); j > 0; j--) {
                    tmp_val |= ((*rRxFifo & 0x00FFFF00) >> obj->sw_rx.regBitOfst) << i;
                }
            }
        } else if (4 == obj->sw_rx.dataWidth) {
            /* word format data buffer */
            for (uint8_t j = (obj->sw_rx.channels == 1 ? 2 : 1); j > 0; j--) {
                tmp_val = *rRxFifo & 0x00FFFFFF;
            }
        }
        *(obj->sw_rx.buffer + obj->sw_rx.bufferCntr) = tmp_val;
        obj->sw_rx.bufferCntr++;
    }
}

int32_t rda_i2s_sem_create(RDA_I2S_SEM_T i2s_sem, int32_t count)
{
    if (TX_SEM == i2s_sem) {
        memset(tx_sem_data, 0, sizeof(tx_sem_data));
        tx_sem_def.semaphore = tx_sem_data;
        tx_sem_id = osSemaphoreCreate(&tx_sem_def, count);
        MBED_ASSERT(tx_sem_id != NULL);
    } else {
        memset(rx_sem_data, 0, sizeof(rx_sem_data));
        rx_sem_def.semaphore = rx_sem_data;
        rx_sem_id = osSemaphoreCreate(&rx_sem_def, count);
        MBED_ASSERT(rx_sem_id != NULL);
    }
    return 0;
}

int32_t rda_i2s_sem_wait(RDA_I2S_SEM_T i2s_sem, uint32_t milliseconds)
{
    int32_t ret;
    if (TX_SEM == i2s_sem) {
        tx_waiting = TRUE;
        ret = osSemaphoreWait(tx_sem_id, milliseconds);
        tx_waiting = FALSE;
    } else {
        rx_waiting = TRUE;
        ret = osSemaphoreWait(rx_sem_id, milliseconds);
        rx_waiting = FALSE;
    }
    return ret;
}

osStatus rda_i2s_sem_release(RDA_I2S_SEM_T i2s_sem)
{
    osStatus status;

    if (TX_SEM == i2s_sem)
        status = osSemaphoreRelease(tx_sem_id);
    else
        status = osSemaphoreRelease(rx_sem_id);

    return status;
}

osStatus rda_i2s_sem_delete(RDA_I2S_SEM_T i2s_sem)
{
    osStatus status;

    if (TX_SEM == i2s_sem) {
        status = osSemaphoreDelete(tx_sem_id);
        tx_sem_id = NULL;
    } else {
        status = osSemaphoreDelete(rx_sem_id);
        rx_sem_id = NULL;
    }

    return status;
}

