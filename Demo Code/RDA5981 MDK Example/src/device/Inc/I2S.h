#ifndef __I2S_H
#define __I2S_H

#include "RDA5991H.h"
#include "GPIO.h"

/** I2S Reg list
 */
#define I2S_CLKGATE_REG1            (RDA_SCU->CLKGATE1)
#define I2S_CLKGATE_REG2            (RDA_SCU->CLKGATE2)
#define I2S_CLK_DIV_REG             (RDA_SCU->I2SCLKDIV)
#define EXIF_MISC_STCFG_REG         (RDA_EXIF->MISCSTCFG)
#define EXIF_MISC_INTCFG_REG        (RDA_EXIF->MISCINTCFG)
#define EXIF_MISC_CFG_REG           (RDA_EXIF->MISCCFG)
#define MEM_CFG_REG                 (RDA_GPIO->MEMCFG)

// I2S HAL 宏
#define DFLT_TXFIFO_ALMOSTEMPTY_WDLEN   (31)
#define DFLT_RXFIFO_ALMOSTFULL_WDLEN    (12)

// 状态寄存器位定义   (RDA_EXIF->MISCSTCFG)
#define I2S_TXFIFO_ALMOSTEMPTY_CLR_BIT  (0x01UL)            //发送FIFO 快空清除掩码
#define I2S_RXFIFO_ALMOSTFULL_CLR_BIT   (0x01UL << 1)       //接收FIFO 快满清除掩码
#define I2S_TXFIFO_ALMOSTEMPTY_ENBIT    (0x01UL << 2)       //发送快满 中断启用   写激活
#define I2S_TXFIFO_WRITEWHENFULL_ENBIT  (0x01UL << 3)       //发送快空 中断启用   写激活
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

//中断配置寄存器 位定义  (RDA_EXIF->MISCINTCFG)
#define I2S_TXINT_ENBIT                 (0x01UL << 11)
#define I2S_RXINT_ENBIT                 (0x01UL << 12)

//引脚配置
#define I2S_MASTER_RX_IOMUX             {I2S_TX_BCLK, I2S_TX_WS, I2S_RX_SD}
#define I2S_MASTER_TX_IOMUX             {I2S_TX_BCLK, I2S_TX_WS, I2S_TX_SD}
#define I2S_SLAVE_RX_IOMUX              {I2S_RX_BCLK, I2S_RX_WS, I2S_RX_SD}

#define RDA_EXIF_INTST									(RDA_EXIF->MISCSTCFG)
#define RDA_EXIF_IRQn										(EXIF_IRQn)

#ifdef __cplusplus
extern "C" {
#endif

//I2S HAL enumeration
typedef enum {
    I2S_64FS  = 0x00,
    I2S_32FS  = 0x01
} I2S_FS_T;
//
typedef enum {
    I2S_WS_NEG  = 0x00,
    I2S_WS_POS  = 0x01
} I2S_WS_POLAR_T;
//
typedef enum {
    I2S_OTHER_M = 0x00,
    I2S_STD_M   = 0x01
} I2S_STD_MODE_T;
//
typedef enum {
    I2S_RIGHT_JM = 0x00,
    I2S_LEFT_JM  = 0x01
} I2S_JUSTIFIED_MODE_T;
//
typedef enum {
    I2S_DL_8b   = 0x00,
    I2S_DL_12b  = 0x01,
    I2S_DL_16b  = 0x02,
    I2S_DL_20b  = 0x03,
    I2S_DL_24b  = 0x04
} I2S_DATA_LEN_T;
//
typedef enum {
    I2S_LSB     = 0x00,
    I2S_MSB     = 0x01
} I2S_MSB_LSB_T;
//
typedef enum {
    I2S_WF_CNTLFT_1W    = 0x00,
    I2S_WF_CNTLFT_2W    = 0x01,
    I2S_WF_CNTLFT_4W    = 0x02,
    I2S_WF_CNTLFT_8W    = 0x03
} I2S_WRFIFO_CNTLEFT_T;
//
typedef enum {
    I2S_MD_MASTER_RX = 0x00,
    I2S_MD_MASTER_TX = 0x01,
    I2S_MD_SLAVE_RX  = 0x02,
    I2S_MD_M_TX_S_RX = 0x03
} RDA_I2S_MODE_T;
//
typedef enum {
    I2S_ST_RESET = 0,
    I2S_ST_READY,
    I2S_ST_BUSY
} RDA_I2S_SW_STATE_T;
//
typedef enum {
    TX_SEM = 0x00,
    RX_SEM = 0x01
} RDA_I2S_SEM_T;
//
typedef enum {
    I2S_0  = (int)RDA_I2S_BASE
} I2SName;
//
typedef struct _i2s_s{
    RDA_I2S_TypeDef *i2s;
}i2s_s;
//I2S HAL structure
typedef struct _RDA_I2S_SW_PARAM_T{
    volatile RDA_I2S_SW_STATE_T state;
    uint32_t *buffer;
    uint16_t bufferCntr;
    uint16_t bufferSize;
    uint8_t  regBitOfst;
    uint8_t  fifoThreshold;
    uint8_t  channel;
    uint8_t  dataWidth;
}RDA_I2S_SW_PARAM_T;

typedef struct _RDA_I2S_HW_PARAM_T{
    uint8_t fs;
    uint8_t ws_polarity;
    uint8_t std_mode;
    uint8_t justified_mode;
    uint8_t data_len;
    uint8_t msb_lsb;
    uint8_t wrfifo_cntleft;
}RDA_I2S_HW_PARAM_T;

typedef struct _i2s_cfg_t{
    RDA_I2S_MODE_T     mode;
    RDA_I2S_HW_PARAM_T tx;
    RDA_I2S_HW_PARAM_T rx;
}i2s_cfg_t;

typedef struct _i2s_t{
    i2s_s 							hw;
    RDA_I2S_SW_PARAM_T	sw_tx;
    RDA_I2S_SW_PARAM_T	sw_rx;
}i2s_t;


void		rda_i2s_irq_handler(uint32_t int_status);
void		rda_i2s_init(i2s_t *obj, PinName tx_bclk, PinName tx_ws, PinName tx_sd,   
	                            PinName rx_bclk, PinName rx_ws, PinName rx_sd, PinName mclk);
void		rda_i2s_enable_tx(i2s_t *obj);


void		rda_i2s_format(i2s_t *obj, i2s_cfg_t *cfg);
uint8_t rda_i2s_set_ws(i2s_t *obj, uint32_t ws, uint32_t ratio);

void		rda_i2s_enable_tx(i2s_t *obj);
void		rda_i2s_enable_rx(i2s_t *obj);
void		rda_i2s_enable_master_rx(void);

void enable_i2s_tx_int(void);
void enable_i2s_rx_int(void);
void i2s_enable_tx_rx_int(void);

void    rda_exif_irq_set(void);
#ifdef __cplusplus
}
#endif


#endif
