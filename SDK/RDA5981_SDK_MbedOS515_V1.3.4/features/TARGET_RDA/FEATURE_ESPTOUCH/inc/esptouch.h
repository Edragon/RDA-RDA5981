#ifndef _ESPTOUCH_H
#define _ESPTOUCH_H

#define ESP_ESPTOUCH_GUIDE_MSG_LEN 512
#define ESP_ESPTOUCH_FIXED_OFFSET 40
#define ESP_ESPTOUCH_EXTRA_HEAD_LEN 9

#define CRC8_TABLE_SIZE         256
#define RECEVEBUF_MAXLEN        200

#define ESPTOUCH_TIMER 100//ms
#define ESPTOUCH_GETHANDLER_TIMER 4*1000//ms
#define ESPTOUCH_GETOFFSET_TIMER 10*1000//s
#define ESPTOUCH_LOCKED_TIMER 30*1000//s

#define READ_REG32(REG)			 *((volatile unsigned int*)(REG))
#define WRITE_REG32(REG, VAL)    ((*(volatile unsigned int*)(REG)) = (unsigned int)(VAL))

#define ESPTOUCH_REPORT_MSG_OFFSET 9
#define ESPTOUCH_REPORT_PORT 18266

enum esptouch_status{
    ESPTOUCH_INIT,
    ESPTOUCH_READY,
    ESPTOUCH_ENABLE,
    ESPTOUCH_DISABLE,
    ESPTOUCH_DONE,
    ESPTOUCH_ERR,
    ESPTOUCH_END,
};

enum wland_esptouch_decode_status {
    ESPTOUCH_CH_INIT,
    ESPTOUCH_CH_LOCKED,
    ESPTOUCH_CH_LOCKING,
    ESPTOUCH_CH_DONE,
};

struct wland_esptouch_offset {
    unsigned short msg[4];
    unsigned char msg_ldpc[4];
    unsigned short len;
    unsigned short len_temp;
};
struct wland_esptouch_analyzer {
    unsigned short msg[3];
};

struct wland_esptouch {
    unsigned char total_len;
    unsigned char SSID_crc;
    unsigned char BSSID_crc;
    unsigned char total_xor;
    unsigned char SSID_len;
    unsigned char SSID_hiden;
    unsigned char key_len;
    unsigned char ip[4];

    char SSID[33];
    char key[64+1];
    enum esptouch_status state;

    void *timer;
    int smart_lock_flag;
    unsigned long smart_lock_time;
    unsigned char BSSID[ETH_ALEN];
    unsigned char channel;
    enum wland_esptouch_decode_status decode_status;
    unsigned short channel_time;
    unsigned short channel_time_all;
    unsigned char current_channel;
    unsigned char channel_bits[20];
    unsigned char channel_bits_len;
    unsigned char smartend;
    unsigned short smart_times;
    int from_ds;
    int ldpc;
    unsigned char buf[RECEVEBUF_MAXLEN+1];
    unsigned char buf_flag[RECEVEBUF_MAXLEN+1];
    unsigned char buf_len;
    unsigned short len_offset;
    struct wland_esptouch_offset offset_msg;
    struct wland_esptouch_offset offset_msg_ldpc;
    struct wland_esptouch_analyzer analyzer_msg;
};

#ifdef __cplusplus
extern "C" {
#endif

/*
 * function: get ssid and passwd from esptouch
 * return: 0:success, else:fail
 */
extern int rda5981_getssid_from_esptouch(char *ssid, char *passwd);
extern void rda5981_stop_esptouch();

/*
 * function: send response to host
 * @ip_addr: ip address
 * @mac_addr: mac address
 * @stack: wifi stack
 */
extern int esptouch_sendrsp_to_host(const char *ip_addr, const char *mac_addr, void *stack);

#ifdef __cplusplus
}
#endif

#endif /* _ESPTOUCH_H */
