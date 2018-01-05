#include "WiFiStackInterface.h"
#include "rda_sys_wrapper.h"
#include "console.h"

extern WiFiStackInterface wifi;
extern unsigned int baudrate;
extern void start_at(void);

void init(void)
{
    int ret;
    ret = rda5981_flash_read_uart(&baudrate);
    if(ret == 0)
        console_set_baudrate(baudrate);
}

/* Description: This is a case to test dual console. 					*/
/* Both consoles can respond to AT command.						*/
/* When user use printf, string is output through uart1. 				*/
/* Users can use console_uart0_printf to printf data through uart0. 	*/

int main()
{
    unsigned int msg;
    int ret, cnt = 1;
    void *main_msgQ = rda_msgQ_create(5);

    init();
    start_at();

	printf("dual console at test start!\r\n");

    wifi.set_msg_queue(main_msgQ);
    while (true) {
        rda_msg_get(main_msgQ, &msg, osWaitForever);
        switch(msg)
        {
            case MAIN_RECONNECT:
                //printf("wifi disconnect!\r\n");
                ret = wifi.disconnect();
                if(ret != 0){
                    //printf("disconnect failed!\r\n");
                    break;
                }
                ret = wifi.reconnect();
                while(ret != 0){
                    if(++cnt>5)
                        break;
                    osDelay(cnt*2*1000);
                    ret = wifi.reconnect();
                };
                cnt = 1;
                break;
            case MAIN_STOP_AP:
                wifi.stop_ap(1);
                break;
            default:
                //printf("unknown msg\r\n");
                break;
        }
    }
}

