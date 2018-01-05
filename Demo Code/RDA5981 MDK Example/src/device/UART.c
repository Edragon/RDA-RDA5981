#include "UART.h"
#include "PIN_MAP.h"


void uart_format(serial_t *obj, int data_bits, SerialParity parity, int stop_bits);
void uart_set_flow_control(serial_t *obj, FlowControl type, PinName rxflow, PinName txflow);
void uart_clear(serial_t *obj);



const PinMap PinMap_UART_TX[] = {
    {PA_1,  UART_0, 0},
    {PB_2,  UART_1, 5},
    {PD_3,  UART_1, 2},
    {NC   , NC    , 0}
};

const PinMap PinMap_UART_RX[] = {
    {PA_0,  UART_0, 0},
    {PB_1,  UART_1, 5},
    {PD_2,  UART_1, 2},
    {NC   , NC    , 0}
};

const PinMap PinMap_UART_RTS[] = {
    {PD_1,  UART_1, 2},
    {NC,    NC,     0}
};

const PinMap PinMap_UART_CTS[] = {
    {PD_0,  UART_1, 2},
    {NC,    NC,     0}
};

void gpio_write(gpio_t *obj, int value) {
    *obj->reg_out = ((value) ? 1 : 0);
}

int gpio_read(gpio_t *obj) {
    return ((*obj->reg_in) ? 1 : 0);
}

int gpio_is_connected(const gpio_t *obj) {
    return obj->pin != (PinName)NC;
}

void uart_init(serial_t *obj, PinName tx, PinName rx) 
{

    // determine the UART to use
    UARTName uart_tx 	= (UARTName)pinmap_peripheral(tx, PinMap_UART_TX);
    UARTName uart_rx 	= (UARTName)pinmap_peripheral(rx, PinMap_UART_RX);
    UARTName uart 		= (UARTName)pinmap_merge(uart_tx, uart_rx);

    switch (uart) 
		{
        case UART_0:
            obj->index = 0;
            break;
        case UART_1:
            obj->index = 1;
            // Enable clock-gating
            UART_CLKGATE_REG |= UART1_CLKEN_MASK;
            break;
    }

    obj->uart = (RDA_UART_TypeDef *)uart;

    // enable fifos and default rx trigger level
    obj->uart->FCR = 0 << 0  //FIFO Enable - 0 = Disables, 1 = Enabled
                   | 0 << 1  // Rx Fifo Reset
                   | 0 << 2  // Tx Fifo Reset
                   | 0 << 6; // Rx irq trigger level - 0 = 1 char, 1 = 4 chars, 2 = 8 chars, 3 = 14 chars

    // 关闭中断
    obj->uart->IER = 0 << 0  // Rx Data available irq enable
                   | 0 << 1  // Tx Fifo empty irq enable
                   | 0 << 2; // Rx Line Status irq enable

    obj->uart->MCR =  1 << 8; //select clock
    obj->uart->FRR = 0x2001;  //tx_trigger = 0x10, rx_trigger = 0x01

    // set default baud rate and format
    // serial_baud  (obj, 921600);
    uart_format(obj, 8, ParityNone, 1);

    // pinout the chosen uart
    pinmap_pinout(tx, PinMap_UART_TX);
    pinmap_pinout(rx, PinMap_UART_RX);

    // set rx/tx pins in PullUp mode
    if (tx != NC) {
        pin_mode(tx, PullUp);
    }
    if (rx != NC) {
        pin_mode(rx, PullUp);
    }

    if ((rx != NC) && (tx != NC)) {
        obj->uart->FCR |= 1 << 0;  //enable fifo
    }

    uart_set_flow_control(obj, FlowControlNone, NC, NC);
    uart_clear(obj);
}

// serial_baud
// set the baud rate, taking in to account the current SystemFrequency
void uart_baud(serial_t *obj, int baudrate) 
{
    uint32_t baud_divisor;
    uint32_t baud_mod;

    baud_divisor  = (AHBBusClock / baudrate) >> 4;
    baud_mod      = (AHBBusClock / baudrate) & 0x0F;

    obj->uart->LCR |= (1 << 7); //enable load devisor register

    obj->uart->DLL = (baud_divisor >> 0) & 0xFF;
    obj->uart->DLH = (baud_divisor >> 8) & 0xFF;
    obj->uart->DL2 = (baud_mod>>1) + ((baud_mod - (baud_mod>>1))<<4);

    obj->uart->LCR &= ~(1 << 7);// after loading, disable load devisor register

}

void uart_format(serial_t *obj, int data_bits, SerialParity parity, int stop_bits)
{
    int parity_enable, parity_select;
    stop_bits -= 1;
    data_bits -= 5;

    switch (parity) 
		{
        case ParityNone: 
				{
					parity_enable = 0; 
					parity_select = 0; 
					break;
				}
        case ParityOdd : 
				{
					parity_enable = 1; 
					parity_select = 0; 
					break;
				}
        case ParityEven: 
				{
					parity_enable = 1; 
					parity_select = 1; 
					break;
				}
        case ParityForced1:
				{
					parity_enable = 1; 
					parity_select = 2; 
					break;
				}
        case ParityForced0:
				{
					parity_enable = 1; 
					parity_select = 3; 
					break;
				}
        default:
            parity_enable = 0; parity_select = 0;
            break;
    }

    obj->uart->LCR |= (data_bits            << 0
                     | stop_bits            << 2
                     | parity_enable        << 3
                     | parity_select        << 4);

}

void uart_break_set(serial_t *obj) 
{
    obj->uart->LCR |= (1 << 6);
}

void uart_break_clear(serial_t *obj) 
{
    obj->uart->LCR &= ~(1 << 6);
}


void uart_clear(serial_t *obj)
{
    obj->uart->FCR = 1 << 0  // FIFO Enable - 0 = Disables, 1 = Enabled
                   | 1 << 1  // rx FIFO reset
                   | 1 << 2; // tx FIFO reset
}

int uart_getc(serial_t *obj) 
{
    int data = 0;
    while ((obj->uart->FSR & 0x00100000))
		{
			;//接受FIFO不为空时
		}
		data = (int)(obj->uart->RBR & 0x00FFUL);  //接收寄存器
    return data;
}

//FSR FIFO 状态寄存器 寄存器描述

//0x000000ff   发送  FIFO 计数  FIFO 最大0x40
//0x0000ff00   接受  FIFO 计数  FIFO 最大0x80

//0x00200000   1=接受FIFO满
//0x00080000   1=发送FIFO满


//0x00100000   1=接受FIFO空
//0x00040000   1=发送FIFO空

//LSR 寄存器描述
//0x00000060   发送的线路状态
//0x00000020   发送的线路状态
//0x00000101   接受时的线路状态
void uart_putc(serial_t *obj, int c) 
{
	while (!(obj->uart->FSR & 0x00040000))
	{
		; //发送FIOF为空时	
	}
  obj->uart->THR = c;
}

void uart_set_flow_control(serial_t *obj, FlowControl type, PinName rxflow, PinName txflow) 
{
	
		//UARTName uart_rts;
		//UARTName uart_cts;    

    // Only UART1 has hardware flow control on RDA5991H
    RDA_UART_TypeDef *uart1 = (uint32_t)obj->uart == (uint32_t)RDA_UART1 ? RDA_UART1 : 0;

		if(uart1 == 0)
		{
				return;
		}
		
    if (FlowControlNone == type) 
		{
        RDA_GPIO->IFCTRL &= ~(0x01UL << 2); //disable flow control
        return;
    }
		
		/*
    // Check type(s) of flow control to use
    uart_rts = (UARTName)pinmap_find_peripheral(rxflow, PinMap_UART_RTS);
    uart_cts = (UARTName)pinmap_find_peripheral(txflow, PinMap_UART_CTS);

    if ((UART_1 == uart_cts) && (NULL != uart1))
    {
        //pinmap_pinout(txflow, PinMap_UART_CTS);
        //gpio_init_in(&uart_data[index].sw_cts, txflow);
    }

    if((UART_1 == uart_rts) && (NULL != uart1))
    {
        //pinmap_pinout(rxflow, PinMap_UART_RTS);
        //gpio_init_out(&uart_data[index].sw_rts, rxflow);
    }

    uart1->MCR				= uart1->MCR | AFCE_MASK; 				//enable auto flow control, in this case we don't have to read and write sw_cts & sw_rts
    uart1->FRR				= (0x3EUL << 0) | (0x3EUL << 9); 	//rts/cts fifo trigger
    RDA_GPIO->IFCTRL |= 0x01UL << 2; 										//enable flow control
		*/
}
