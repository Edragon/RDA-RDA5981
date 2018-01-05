#ifndef _SMART_CONFIG_H
#define _SMART_CONFIG_H

#define ESP_SMART_CONFIG_GUIDE_MSG_LEN 517
#define ESP_SMART_CONFIG_FIXED_OFFSET 45
#define ESP_SMART_CONFIG_EXTRA_HEAD_LEN 9

#define CRC8_TABLE_SIZE         256
#define RECEVEBUF_MAXLEN        200

#define SMART_CONFIG_TIMER 120//ms
#define SMART_CONFIG_GETHANDLER_TIMER 4*1000//ms
#define SMART_CONFIG_GETOFFSET_TIMER 10*1000//s
#define SMART_CONFIG_LOCKED_TIMER 30*1000//s

#define READ_REG32(REG)			 *((volatile unsigned int*)(REG))
#define WRITE_REG32(REG, VAL)    ((*(volatile unsigned int*)(REG)) = (unsigned int)(VAL))

#define SMARTCONFIG_REPORT_MSG_OFFSET 9
#define SMARTCONFIG_REPORT_PORT 18266

enum wland_smartconfig_status{
    SMARTCONFIG_INIT,
    SMARTCONFIG_READY,
    SMARTCONFIG_ENABLE,
    SMARTCONFIG_DISABLE,
    SMARTCONFIG_DONE,
    SMARTCONFIG_ERR,
    SMARTCONFIG_END,
};

enum wland_smartconfig_decode_status {
    SMART_CH_INIT,
    SMART_CH_LOCKED,
    SMART_CH_LOCKING,
    SMART_CH_DONE,
};

struct wland_smartconfig_offset {
    unsigned short msg[4];
    unsigned char msg_ldpc[4];
    unsigned short len;
    unsigned short len_temp;
};
struct wland_smartconfig_analyzer {
    unsigned short msg[3];
};

struct wland_smartconfig {
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
    enum wland_smartconfig_status state;

    void *timer;
    int smart_lock_flag;
    unsigned long smart_lock_time;
    unsigned char BSSID[ETH_ALEN];
    unsigned char channel;
    enum wland_smartconfig_decode_status decode_status;
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
    struct wland_smartconfig_offset offset_msg;
    struct wland_smartconfig_offset offset_msg_ldpc;
    struct wland_smartconfig_analyzer analyzer_msg;
};

#ifdef __cplusplus
extern "C" {
#endif

/*
 * function: get ssid and passwd from smart config
 * return: 0:success, else:fail
 */
extern int rda5981_getssid_from_smartconfig(char *ssid, char *passwd);
extern void rda5981_stop_smartconfig();
/*
 * function: send response to host
 * @ip_addr: ip address
 * @mac_addr: mac address
 * @stack: wifi stack
 */
extern int rda5981_sendrsp_to_host(const char *ip_addr, const char *mac_addr, void *stack);

#ifdef __cplusplus
}
#endif

#endif /* _SMART_CONFIG_H */
