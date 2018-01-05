#include "mbed.h"
#include "rtos.h"
#include "WiFiStackInterface.h"
#include "rda_sys_wrapper.h"
#include "rda5981_sniffer.h"


/* Frame Type and Subtype Codes (6-bit) */
enum TYPESUBTYPE_T{
    ASSOC_REQ             = 0x00,
    ASSOC_RSP             = 0x10,
    REASSOC_REQ           = 0x20,
    REASSOC_RSP           = 0x30,
    PROBE_REQ             = 0x40,
    PROBE_RSP             = 0x50,
    BEACON                = 0x80,
    ATIM                  = 0x90,
    DISASOC               = 0xA0,
    AUTH                  = 0xB0,
    DEAUTH                = 0xC0,
    ACTION                = 0xD0,
    PS_POLL               = 0xA4,
    RTS                   = 0xB4,
    CTS                   = 0xC4,
    ACK                   = 0xD4,
    CFEND                 = 0xE4,
    CFEND_ACK             = 0xF4,
    DATA                  = 0x08,
    DATA_ACK              = 0x18,
    DATA_POLL             = 0x28,
    DATA_POLL_ACK         = 0x38,
    NULL_FRAME            = 0x48,
    CFACK                 = 0x58,
    CFPOLL                = 0x68,
    CFPOLL_ACK            = 0x78,
    QOS_DATA              = 0x88,
    QOS_DATA_ACK          = 0x98,
    QOS_DATA_POLL         = 0xA8,
    QOS_DATA_POLL_ACK     = 0xB8,
    QOS_NULL_FRAME        = 0xC8,
    QOS_CFPOLL            = 0xE8,
    QOS_CFPOLL_ACK        = 0xF8,
    BLOCKACK_REQ          = 0x84,
    BLOCKACK              = 0x94,
};


int my_smartconfig_handler(unsigned short data_len, void *data)
{
    unsigned short seq_tmp;
    char type, *typc_c;
    char *frame = (char *)data;
    if (data==NULL) {
        printf("ldpc:%d\n", data_len);
    } else {
        seq_tmp = (frame[22] | (frame[23]<<8))>>4;
        type = frame[0] & 0xfc;
        switch(type) {
            case BEACON:
                typc_c = "BEACON";
                break;
            case QOS_DATA:
            case DATA:
                typc_c = "DATA";
                //wland_dump_frame_t("handler data", (unsigned char*)data, data_len);
                break;
            case PROBE_REQ:
                typc_c = "--------PROBE_REQ";
                break;
            case PROBE_RSP:
                typc_c = "*****PROBE_RSP";
                break;
            default:
                typc_c = "unknown";
                printf("unknown type:0x%x\n", type);
          }
        printf("%s, seq:%d, len:%d\n", typc_c, seq_tmp, data_len);
#if 0
        {
            int i;
            for (i=0; i<data_len; ++i)
                printf("%02x ", frame[i]);
            printf("\n");
        }
#endif
    }
    return 0;
}

char data[]=
{0x80, 0x00, //beacon
 0x00, 0x00, //duration
 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, //destination
 0x3a, 0xcc, 0x0b, 0x06, 0x21, 0xea, //source
 0x00, 0x5a, 0x39, 0x2f, 0x5b, 0x90, //BSSID
 0x80, 0x08, //seq num

 0x39, 0xd0, 0xb6, 0x00, 0x00, 0x00, 0x00, 0x00,
 0x64, 0x00, 0x31, 0x84, 0x00, 0x01, 0x79, 0x01,
 0x08, 0x82, 0x84, 0x8b, 0x96, 0x0c, 0x12, 0x18,
 0x24, 0x03, 0x01, 0x06, 0x2a, 0x01, 0x00, 0x32,
 0x04, 0x30, 0x48, 0x60, 0x6c, 0x05, 0x04, 0x00,
 0x01, 0x00, 0x00, 0xdd, 0x18, 0x00, 0x50, 0xf2,
 0x02, 0x01, 0x01, 0x00, 0x00, 0x03, 0x43, 0x00,
 0x00, 0x27, 0x43, 0x00, 0x00, 0x42, 0x32, 0x5e,
 0x00, 0x62, 0x21, 0x2f, 0x00, 0x30, 0x14, 0x01,
 0x00, 0x00, 0x0f, 0xac, 0x04, 0x01, 0x00, 0x00,
 0x0f, 0xac, 0x04, 0x01, 0x00, 0x00, 0x0f, 0xac,
 0x02, 0x00, 0x00, 0x2d, 0x1a, 0x6e, 0x10, 0x1f,
 0xff, 0x00, 0x00, 0x00, 0x01, 0x00, 0x00, 0x00,
 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
 0x06, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x3d,
 0x16, 0x06, 0x08};

int main() {
    int ret, cnt = 1;
    WiFiStackInterface wifi;
    const char *ip_addr;
    unsigned int msg;
    void *main_msgQ = rda_msgQ_create(5);
    printf("Start wifistack test...\r\n");
#if 0
    //scan example
    rda5981_scan_result *bss_list = NULL;
    int scan_res, cnt;
    scan_res = wifi.scan(NULL, 0);
    bss_list = (rda5981_scan_result *)malloc(scan_res * sizeof(rda5981_scan_result));
    memset(bss_list, 0, scan_res * sizeof(rda5981_scan_result));
    if (bss_list == NULL) {
        printf("malloc buf fail\r\n");
        return -1;
    }

    cnt = wifi.scan_result(bss_list, scan_res);
    printf("##########scan return :%d\n", cnt);
    free(bss_list);
#endif
    //connect example
    wifi.set_msg_queue(main_msgQ);
    wifi.set_sta_listen_interval(2);//not necessary
    wifi.set_sta_link_loss_time(50);//not necessary
    wifi.scan(NULL, 0);
    wifi.connect("ASUS", "qqqqqqqq", NULL, NSAPI_SECURITY_NONE);
    //wifi.start_ap("a", NULL, 6);

    ip_addr = wifi.get_ip_address();
    if (ip_addr) {
        printf("Client IP Address is %s\r\n", ip_addr);
    } else {
        printf("No Client IP Address\r\n");
    }

    osDelay(1000);
    rda5981_send_nulldata(1);
    rda5981_set_channel(11);
    rda5981_send_rawdata(data, sizeof(data));
    rda5981_send_rawdata(data, sizeof(data));
    rda5981_send_rawdata(data, sizeof(data));
    rda5981_send_rawdata(data, sizeof(data));
    rda5981_send_rawdata(data, sizeof(data));
    //printf("channel %d\r\n", wifi.get_channel());
    rda5981_set_channel(wifi.get_channel());
    rda5981_send_nulldata(0);

    osDelay(3000);
    rda5981_enable_sniffer(my_smartconfig_handler);
    rda5981_set_filter(1, 1, 0x3fe77);
    osDelay(10000);
    rda5981_disable_sniffer();

    while (true) {
        rda_msg_get(main_msgQ, &msg, osWaitForever);
        switch(msg)
        {
            case MAIN_RECONNECT: {
                printf("wifi disconnect!\r\n");
                ret = wifi.disconnect();
                if(ret != 0){
                    printf("disconnect failed!\r\n");
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
            }
            case MAIN_STOP_AP:
                wifi.stop_ap(1);
                break;
            default:
                printf("unknown msg\r\n");
                break;
        }
    }
}
