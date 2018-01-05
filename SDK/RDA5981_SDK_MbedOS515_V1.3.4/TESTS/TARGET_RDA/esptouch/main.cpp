#include "mbed.h"
#include "rtos.h"
#include "WiFiStackInterface.h"
#include "gpadckey.h"
#include "esptouch.h"

#define ESPTOUCH_USE_KEY_DOWN

WiFiStackInterface wifi;

#ifdef ESPTOUCH_USE_KEY_DOWN
GpadcKey esptouch_key_down(KEY_A5);
Timer timer;
unsigned int falltime;

void wland_esptouch_irq_fall()
{
    timer.start();
    falltime = timer.read_ms();
}

void wland_esptouch_irq_rise()
{
    unsigned int time_now = timer.read_ms();
    if ((time_now - falltime) > 3000) {  //about 5s
        printf("clear esptouch data\r\n");
        rda5981_flash_erase_sta_data();         //long press for recording
    }
    timer.stop();
}

#else /*ESPTOUCH_USE_KEY_DOWN*/
InterruptIn esptouch_key(GPIO_PIN4);
void wland_esptouch_irq()
{
    rda5981_flash_erase_sta_data();
}
#endif /*ESPTOUCH_USE_KEY_DOWN*/

void rda5991h_esptouch_irq_init()
{
    printf("%s\r\n", __func__);
#ifdef ESPTOUCH_USE_KEY_DOWN
    esptouch_key_down.fall(&wland_esptouch_irq_fall);
    esptouch_key_down.rise(&wland_esptouch_irq_rise);
#else /*ESPTOUCH_USE_KEY_DOWN*/
    esptouch_key.fall(&wland_esptouch_irq);
#endif /*ESPTOUCH_USE_KEY_DOWN*/
}

int main() {
    int ret;
    char SSID[32+1];
    char passwd[64+1];
    int esptouch_flag = 0;
    const char *ip;
    const char *mac;

    printf("\nstart esptouch test\r\n");

    ret = wifi.scan(NULL, 0);//necessary

    rda5991h_esptouch_irq_init();

    //get ssid from flash
    ret = rda5981_flash_read_sta_data(SSID, passwd);
    if (0 && ret == 0 && strlen(SSID)) {
        printf("get ssid from flash: ssid:%s, pass:%s\r\n", SSID, passwd);
    } else {//need to esptouch
        ret = rda5981_getssid_from_esptouch(SSID, passwd);
        if (ret) {
            printf("esptouch fail, ret: %d \r\n", ret);
            return ret;
        } else {
            printf("esptouch success \r\n");
            printf("ssid = \"%s\", pwd = \"%s\" \r\n", SSID, passwd);
            esptouch_flag = 1;
        }
    }

    ret = wifi.connect(SSID, passwd, NULL, NSAPI_SECURITY_NONE);

    if (ret == 0) {
        ip = wifi.get_ip_address();
        mac = wifi.get_mac_address();
        printf("connect to %s success, ip: %s\r\n", SSID, ip);
    } else {
        printf("connect to %s fail\r\n", SSID);
    }

    if (esptouch_flag && !ret) {
        rda5981_flash_write_sta_data(SSID, passwd);
        ret = esptouch_sendrsp_to_host(ip, mac, (void *)&wifi);
        if (ret <= 0) {
            printf("send response to host fail\r\n");
        }
    }

    while (true) {
    }
}

