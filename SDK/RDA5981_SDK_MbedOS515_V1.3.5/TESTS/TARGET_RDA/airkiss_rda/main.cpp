#include "mbed.h"
#include "rtos.h"
#include "inet.h"
#include "WiFiStackInterface.h"
#include "rda_airkiss.h"
#include "gpadckey.h"

WiFiStackInterface wifi;

/* airkiss reset */
GpadcKey airkiss_reset_key(KEY_A5);
Timer airkiss_reset_timer;
unsigned int falltime;
void airkiss_reset_irq_fall()
{
    airkiss_reset_timer.start();
    falltime = airkiss_reset_timer.read_ms();
}

void airkiss_reset_irq_rise()
{
    unsigned int time_now = airkiss_reset_timer.read_ms();
    if((time_now - falltime) > 3000) {   //about 5s
        printf("clear ssid saved in flash\r\n");
        rda5981_flash_erase_sta_data();         //long press for recording
    }
    airkiss_reset_timer.stop();
}

void rda5991h_smartlink_irq_init()
{
    printf("%s\r\n", __func__);
    airkiss_reset_key.fall(&airkiss_reset_irq_fall);
    airkiss_reset_key.rise(&airkiss_reset_irq_rise);
}

int main() {
    int ret;
    int airkiss_send_rsp = 0;
    char ssid[32+1];
    char passwd[64+1];

    rda5991h_smartlink_irq_init();

    ret = rda5981_flash_read_sta_data(ssid, passwd);
    if (ret == 0 && strlen(ssid)) {//get ssid from flash
        printf("get ssid from flash: ssid:%s, pass:%s\r\n",
            ssid, passwd);
    } else {//need to airkiss
        printf("start airkiss, version:%s\n", airkiss_version());
        ret = get_ssid_pw_from_airkiss(ssid, passwd);
        //you can stop airkiss by rda5981_stop_airkiss
        if (ret) {
            printf("airkiss fail, ret: %d \r\n", ret);
            return ret;
        } else {
            printf("airkiss success! ssid = \"%s\", pwd = \"%s\" \r\n", ssid, passwd);
            airkiss_send_rsp = 1;
        }
    }

    ret = wifi.connect(ssid, passwd, NULL, NSAPI_SECURITY_NONE);
    if (ret == 0) {
        printf("connect to %s success, ip %s\r\n", ssid, wifi.get_ip_address());
    } else {
        printf("connect to %s fail\r\n", ssid);
    }

    if (airkiss_send_rsp && !ret) {
        rda5981_flash_write_sta_data(ssid, passwd);
        ret = airkiss_sendrsp_to_host((void *)&wifi);
        if (ret <= 0) {
            printf("send response to host fail\r\n");
        }
    }
    while (true) {
    }
}


