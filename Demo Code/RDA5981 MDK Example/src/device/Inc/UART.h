#ifndef __UART_H
#define __UART_H

#include "RDA5991H.h"
#include "GPIO.h"

#define SERIAL_EVENT_TX_SHIFT (2)
#define SERIAL_EVENT_RX_SHIFT (8)

#define SERIAL_EVENT_TX_MASK (0x00FC)
#define SERIAL_EVENT_RX_MASK (0x3F00)

#define SERIAL_EVENT_ERROR (1 << 1)

//SerialTXEvents Serial TX Events Macros
#define SERIAL_EVENT_TX_COMPLETE (1 << (SERIAL_EVENT_TX_SHIFT + 0))
#define SERIAL_EVENT_TX_ALL      (SERIAL_EVENT_TX_COMPLETE)


//SerialRXEvents Serial RX Events Macros
#define SERIAL_EVENT_RX_COMPLETE        (1 << (SERIAL_EVENT_RX_SHIFT + 0))
#define SERIAL_EVENT_RX_OVERRUN_ERROR   (1 << (SERIAL_EVENT_RX_SHIFT + 1))
#define SERIAL_EVENT_RX_FRAMING_ERROR   (1 << (SERIAL_EVENT_RX_SHIFT + 2))
#define SERIAL_EVENT_RX_PARITY_ERROR    (1 << (SERIAL_EVENT_RX_SHIFT + 3))
#define SERIAL_EVENT_RX_OVERFLOW        (1 << (SERIAL_EVENT_RX_SHIFT + 4))
#define SERIAL_EVENT_RX_CHARACTER_MATCH (1 << (SERIAL_EVENT_RX_SHIFT + 5))
#define SERIAL_EVENT_RX_ALL             (SERIAL_EVENT_RX_OVERFLOW | SERIAL_EVENT_RX_PARITY_ERROR | \
                                         SERIAL_EVENT_RX_FRAMING_ERROR | SERIAL_EVENT_RX_OVERRUN_ERROR | \
                                         SERIAL_EVENT_RX_COMPLETE | SERIAL_EVENT_RX_CHARACTER_MATCH)

#define SERIAL_RESERVED_CHAR_MATCH (255)


#define UART_NUM    2

#define UART_CLKGATE_REG        (RDA_SCU->CLKGATE0)
#define UART1_CLKEN_MASK        (0x01UL << 21)

#define RXFIFO_EMPTY_MASK       (0x01UL << 0)
#define TXFIFO_FULL_MASK        (0x01UL << 19)
#define AFCE_MASK               (0x01UL << 5)


#ifdef __cplusplus
extern "C" {
#endif
	
typedef enum {
    UART_0 = (int)RDA_UART0_BASE,
    UART_1 = (int)RDA_UART1_BASE
} UARTName;

typedef enum {
    ParityNone = 0,
    ParityOdd = 1,
    ParityEven = 2,
    ParityForced1 = 3,
    ParityForced0 = 4
} SerialParity;

typedef enum {
    RxIrq,
    TxIrq
} SerialIrq;

typedef enum {
    FlowControlNone,
    FlowControlRTS,
    FlowControlCTS,
    FlowControlRTSCTS
} FlowControl;

typedef struct _serial_t{
    RDA_UART_TypeDef *uart;
    int index;
}serial_t;

/*
typedef struct _buffer_s{
    void    *buffer; // the pointer to a buffer
    size_t   length; // the buffer length
    size_t   pos;    // actual buffer position
    uint8_t  width;  // The buffer unit width (8, 16, 32, 64), used for proper *buffer casting
}buffer_s;


typedef struct _serial_global_data_s{
    uint32_t serial_irq_id;
    gpio_t sw_rts, sw_cts;
    uint8_t count, rx_irq_set_flow, rx_irq_set_api;
}serial_global_data_s;
*/

//typedef void (*uart_irq_handler)(uint32_t id, SerialIrq event);



void uart_init(serial_t *obj, PinName tx, PinName rx);
void uart_baud(serial_t *obj, int baudrate);
void uart_format(serial_t *obj, int data_bits, SerialParity parity, int stop_bits);

void uart_putc(serial_t *obj, int c);
int  uart_getc(serial_t *obj);

#ifdef __cplusplus
}
#endif

#endif
