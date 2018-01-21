#include "RDA5991H.h"
#include "UART.h"

serial_t UartObj;
uint32_t i;

uint32_t buf[30];

int rda_ccfg_boot1(void)
{
	//for(i=0;i<0x003ffff0;i++) ;
	return 1;
}

void main1(void)
{
		//--------------------------------------------------
		//串口
		UartObj.uart = RDA_UART0;
		UartObj.index = 0;
		
		uart_init(&UartObj, UART0_TX, UART0_RX);
		uart_baud(&UartObj, 9600);
		uart_format(&UartObj, 8, ParityNone, 1);
		
		//---------------------------------------------------
		
		while(1)
		{
			uart_putc(&UartObj, uart_getc(&UartObj)); //收到的发出去
		}
	
}
