#include "mbed.h"
#include "rtos.h"
#include "inet.h"
#include "WiFiStackInterface.h"
#include "rda5981_sniffer.h"
#include "airkiss.h"
#include "gpadckey.h"

WiFiStackInterface wifi;

char ssid[32];
char passwd[64];
uint8_t random;
Semaphore airkiss_done(0);

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

/* airkiss data */
const airkiss_config_t akconf =
{
	(airkiss_memset_fn)&memset,
	(airkiss_memcpy_fn)&memcpy,
	(airkiss_memcmp_fn)&memcmp,
	(airkiss_printf_fn)&printf
};
airkiss_context_t akcontex;

uint8_t my_scan_channel[15];
int cur_channel = 0;
int to_ds = 1;
int from_ds = 0;
int mgm_frame = 0;
static void time_callback(void const *p)
{
    int i;
    do {
        cur_channel++;
    	if (cur_channel > 13) {
    		cur_channel = 1;
            to_ds = to_ds?0:1;
            from_ds = from_ds?0:1;
    	}
    } while (my_scan_channel[cur_channel] == 0);
    rda5981_start_sniffer(cur_channel, to_ds, from_ds, mgm_frame, 0);
    airkiss_change_channel(&akcontex);
}

RtosTimer timer(time_callback, osTimerPeriodic, 0);

static int send_rsp_to_apk(uint8_t random)
{
    int i;
    int ret;
    int udp_broadcast = 1;
    UDPSocket udp_socket;
    ip_addr_t ip4_addr;
    char host_ip_addr[16];

    memset((u8_t *)(&ip4_addr), 0xff, 4);
    strcpy(host_ip_addr, inet_ntoa(ip4_addr));

    printf("send response to host:%s\n", host_ip_addr);

    ret = udp_socket.open(&wifi);
    if (ret) {
        printf("open socket error:%d\r\n", ret);
        return ret;
    }

    ret = udp_socket.setsockopt(0,NSAPI_UDP_BROADCAST,&udp_broadcast,sizeof(udp_broadcast));
    if (ret) {
        printf("setsockopt error:%d\r\n", ret);
        return ret;
    }

    for (i=0; i<30; ++i) {
        ret = udp_socket.sendto(host_ip_addr, 10000, &random, 1);
        if (ret <= 0)
            printf("send rsp fail%d\n", ret);
        wait_us(100 * 1000);
    }
    printf("send response to host done\n");
    return ret;
}

static void airkiss_finish(void)
{
	int err;
	airkiss_result_t result;
	err = airkiss_get_result(&akcontex, &result);
	if (err == 0)
	{
		printf("airkiss_get_result() ok!");
        printf("ssid = \"%s\", pwd = \"%s\", ssid_length = %d, \"pwd_length = %d, random = 0x%02x\r\n",
        result.ssid, result.pwd, result.ssid_length, result.pwd_length, result.random);
        rda5981_stop_sniffer();
        strcpy(ssid, result.ssid);
        strcpy(passwd, result.pwd);
        random = result.random;
        airkiss_done.release();
	}
	else
	{
		printf("airkiss_get_result() failed !\r\n");
	}
}

static void wifi_promiscuous_rx(uint8_t *buf, uint16_t len)
{
	int ret;
	ret = airkiss_recv(&akcontex, buf, len);
	if ( ret == AIRKISS_STATUS_CHANNEL_LOCKED)
        timer.stop();
	else if ( ret == AIRKISS_STATUS_COMPLETE )
	{
		airkiss_finish();
	}
}

void start_airkiss(void)
{
    int ret;

    ret = airkiss_init(&akcontex, &akconf);
    if (ret < 0)
    {
        printf("Airkiss init failed!\r\n");
        return;
    }
}

int my_smartconfig_handler(unsigned short data_len, void *data)
{
    static unsigned short seq_num = 0, seq_tmp;

    char *frame = (char *)data;

    seq_tmp = (frame[22] | (frame[23]<<8))>>4;
    if (seq_tmp == seq_num)
        return 0;
    else
        seq_num = seq_tmp;
    wifi_promiscuous_rx((uint8_t*)data, data_len);
    return 0;
}

int main() {
    int i;
    int ret;
    int airkiss_send_rsp = 0;
    rda5981_scan_result scan_result[30];

    memset(my_scan_channel, 0, sizeof(my_scan_channel));

    ret = wifi.scan(NULL, 0);//necessary
    ret = wifi.scan_result(scan_result, 30);
    printf("scan result:%d\n", ret);
    for(i=0; i<ret; ++i) {
        if (scan_result[i].channel<=13) {
            printf("i:%d, channel%d\n", i, scan_result[i].channel);
            my_scan_channel[scan_result[i].channel] = 1;
        }
    }

    rda5991h_smartlink_irq_init();

    ret = rda5981_flash_read_sta_data(ssid, passwd);
    if (ret==0 && strlen(ssid)) {//get ssid from flash
        printf("get ssid from flash: ssid:%s, pass:%s\r\n",
            ssid, passwd);
    } else {//need to smartconfig
        printf("start airkiss, version:%s\n", airkiss_version());
        rda5981_enable_sniffer(my_smartconfig_handler);

        start_airkiss();

        timer.start(200);

        airkiss_done.wait();
        airkiss_send_rsp = 1;
        wait_us(500 * 1000);
    }

    ret = wifi.connect(ssid, passwd, NULL, NSAPI_SECURITY_NONE);
    if (ret==0) {
        printf("connect to %s success, ip %s\r\n", ssid, wifi.get_ip_address());
    } else {
        printf("connect to %s fail\r\n", ssid);
    }

    if (airkiss_send_rsp) {
        rda5981_flash_write_sta_data(ssid, passwd);
        send_rsp_to_apk(random);
    }

    while (true) {
    }
}


