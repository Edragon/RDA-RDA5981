#ifndef __GPIO_H
#define __GPIO_H

#include "core_cm4.h"
#include <stddef.h>
#include "RDA5991H.h"
#include "cmsis.h"
#include "GPIO.h"
#include "UART.h"
#include "I2S.h"

#ifndef BOOL
#define BOOL uint8_t
#endif

#ifndef TRUE
#define TRUE 1
#endif

#ifndef FALSE
#define FALSE 0
#endif

#ifdef __cplusplus
extern "C" {
#endif
	
/*
typedef struct _gpio_irq_s 
{
    uint16_t ch;
    uint8_t  flagR;
    uint8_t  flagF;
}gpio_irq_s;

typedef struct _port_s 
{
    PortName port;
    uint32_t mask;
    __IO uint32_t *reg_out;
    __I  uint32_t *reg_in;
    __IO uint32_t *reg_dir;
}port_s;

typedef struct _pwmout_s 
{
    uint32_t channel;
    uint32_t base_clk;
    uint32_t period_ticks;
    uint32_t pulsewidth_ticks;
    __IO uint32_t *CFGR;
}pwmout_s;

typedef struct _wdt_s 
{
    RDA_WDT_TypeDef *wdt;
}wdt_s;

typedef struct analogin_s 
{
    ADCName adc;
}_analogin_s;

typedef struct gpadc_s 
{
    GPADCName ch;
    PinName   pin;
}gpadc_s;


struct dac_s {
    DACName dac;
};

typedef struct _i2c_s 
{
    RDA_I2C0_TypeDef *i2c;
}i2c_s;

typedef struct _spi_s{
    RDA_SPI_TypeDef *spi;
    uint8_t bit_ofst[2];
}spi_s;
*/
/*
typedef enum {
    I2C_0  = (int)RDA_I2C0_BASE
} I2CName;

typedef enum {
    SPI_0  = (int)RDA_SPI0_BASE
} SPIName;



typedef enum {
    WDT_0  = (int)RDA_WDT_BASE
} WDTName;

typedef enum {
    PWM_0 = 0,
    PWM_1,
    PWM_2,
    PWM_3,
    PWM_4,
    PWM_5,
    PWM_6,
    PWM_7
} PWMName;

typedef enum {
    ADC0_0 = 0,
    ADC0_1,
    ADC0_2
} ADCName;

typedef enum {
    GPADC0_0 = 0,
    GPADC0_1
} GPADCName;
*/


#ifdef __cplusplus
}
#endif

#endif

