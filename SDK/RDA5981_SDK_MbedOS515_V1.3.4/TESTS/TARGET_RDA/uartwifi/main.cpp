#include "WiFiStackInterface.h"
#include "rda_sys_wrapper.h"
#include "console.h"

extern WiFiStackInterface wifi;
extern unsigned int baudrate;
extern char conn_flag;
extern void start_at(void);

void init(void)
{
    int ret;
    ret = rda5981_flash_read_uart(&baudrate);
    if(ret == 0)
        console_set_baudrate(baudrate);
}

int main()
{
    unsigned int msg;
    int ret, cnt = 1;
    void *main_msgQ = rda_msgQ_create(5);

    init();
    start_at();

    wifi.set_msg_queue(main_msgQ);
    while (true) {
        rda_msg_get(main_msgQ, &msg, osWaitForever);
        switch(msg)
        {
            case MAIN_RECONNECT:
                //printf("wifi disconnect!\r\n");
                conn_flag = 0;
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
                if(cnt <= 5)
                    conn_flag = 1;
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

