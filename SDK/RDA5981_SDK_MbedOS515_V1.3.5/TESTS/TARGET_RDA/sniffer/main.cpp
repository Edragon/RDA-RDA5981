#include "mbed.h"
#include "rtos.h"
#include "WiFiStackInterface.h"
#include "rda5981_sniffer.h"
#include "console.h"

static WiFiStackInterface wifi;
static uint8_t my_scan_channel[15];


/* Frame Type and Subtype Codes (6-bit) */
enum TYPESUBTYPE_T
{
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
    BLOCKACK              = 0x94
};

int my_smartconfig_handler(unsigned short data_len, void *data)
{
    unsigned short seq_tmp;
    char type, *typc_c;
    char *frame = (char *)data;
    unsigned short len_fromds;
    if (data==NULL) {
        printf("ldpc:%d\n", data_len);
    } else if (data_len == 2) {
        len_fromds = *((unsigned short *)data);
        printf("rda from ds 11m pkt:%d\n", len_fromds);
    } else {
        seq_tmp = (frame[22] | (frame[23]<<8))>>4;
        type = frame[0] & 0xfc;
        switch(type) {
            case BEACON:
                typc_c = "BEACON";
                break;
            case QOS_DATA:
            case DATA:
                if ((frame[1]&0x3) == 0x1)
                    typc_c = "TODS_DATA";
                else
                    typc_c = "FROMDS_DATA";
                break;
            case PROBE_REQ:
                typc_c = "PROBE_REQ";
                break;
            case PROBE_RSP:
                typc_c = "PROBE_RSP";
                break;
            case AUTH:
                typc_c = "AUTH";
                break;
            case ASSOC_REQ:
                typc_c = "ASSOC_REQ";
                break;
            case ASSOC_RSP:
                typc_c = "ASSOC_RSP";
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

int main() {
    int i;
    int ret;
    rda5981_scan_result scan_result[30];

    console_init();

    memset(my_scan_channel, 0, sizeof(my_scan_channel));

    ret = wifi.scan(NULL, 0);//necessary
    //ret = wifi.scan(NULL, 0);//necessary
    //ret = wifi.scan(NULL, 0);//necessary
    //ret = wifi.scan_result(scan_result, 30);
    printf("scan result:%d\n", ret);
    for(i=0; i<ret; ++i) {
        if (scan_result[i].channel<=13) {
            printf("i:%d, channel%d\n", i, scan_result[i].channel);
            my_scan_channel[scan_result[i].channel] = 1;
        }
    }

    printf("start smartlink\n");

    rda5981_enable_sniffer(my_smartconfig_handler);

    rda5981_start_sniffer(6, 1, 0, 0, 0);
    //rda5981_sniffer_enable_fcs();

    while (true) {
        wait_us(30*1000*1000);
    }
    rda5981_stop_sniffer();
}


