#include <stdio.h>
#include "mbed.h"
#include "rtos.h"
#include "console.h"
#include "WiFiStackInterface.h"
#include "rda_wdt_api.h"
#include "rda_sleep_api.h"
#include "rda_ccfg_api.h"
#include "Thread.h"
#include "cmsis_os.h"
#include "gpadckey.h"
#include "at.h"
#include "ping/ping.h"
#include "smart_config.h"
#include "rda_airkiss.h"
#include "inet.h"
#include "lwip/api.h"
#include "rda_sys_wrapper.h"
#include "SDMMCFileSystem.h"

#define SOCKET_NUM 4
#define SEND_LIMIT  1460
#define RECV_LIMIT  1460
#define IP_LENGTH 16

#define MAC_FORMAT                          "%02x:%02x:%02x:%02x:%02x:%02x"
#define MAC2STR(ea)                  (ea)[0], (ea)[1], (ea)[2], (ea)[3], (ea)[4], (ea)[5]


typedef enum{
    TCP,
    UDP,
}socket_type_t;

typedef struct{
    void *socket;
    int type;
    char SERVER_ADDR[20];
    int SERVER_PORT;
    int LOCAL_PORT; //used for UDP
    int used;
    SocketAddress address;
    void *threadid;
}rda_socket_t;

extern unsigned char rda_mac_addr[6];
extern unsigned int os_time;
extern unsigned int echo_flag;

/* default ap net info*/
extern char mbed_ip_addr[4];
extern char mbed_msk_addr[4];
extern char mbed_gw_addr[4];
extern char mbed_dhcp_start[4];
extern char mbed_dhcp_end[4];

extern int wland_dbg_dump;
extern int wland_dbg_level;
extern int wpa_dbg_dump;
extern int wpa_dbg_level;

WiFiStackInterface wifi;
static char ssid[32+1];
static char pw[64+1];
static char ssid_ap[32+1];
static char pw_ap[64+1];
static char *version = "**********RDA Software Version rdawifi_V3.0_280**********";
char conn_flag = 0;
static char ap_flag = 0;
static char ap_net_flag = 0;
static char fixip_flag = 0;
static char trans_index = 0xff;
static rda_socket_t rda_socket[SOCKET_NUM];
unsigned int baudrate = 921600;

/* SDMMC Pins: clk, cmd, d0, d1, d2, d3 */
//SDMMCFileSystem sdc(PB_9, PB_0, PB_6, PB_7, PC_0, PC_1, "sdc");  // for old EVB
SDMMCFileSystem sdc(PB_9, PB_0, PB_3, PB_7, PC_0, PC_1, "sdc");  // for RDA5991H_HDK_V1.0

/* Smartconfig */
#define SMARTCONFIG_USE_KEY_DOWN

#ifdef SMARTCONFIG_USE_KEY_DOWN
GpadcKey smartconfig_key_down(KEY_A4);
Timer timer;
unsigned int falltime;
void wland_smartlink_irq_fall()
{
    falltime = timer.read_ms();
    rda5981_flash_erase_sta_data();
}

void wland_smartlink_irq_rise()
{
    if((timer.read_ms() - falltime) > 1000) {   //1s
        //rda5981_flash_erase_sta_data();         //long press for recording
    }
}

#else /*SMARTCONFIG_USE_KEY_DOWN*/
InterruptIn smartconfig_key(GPIO_PIN4);
void wland_smartlink_irq()
{
    rda5981_flash_erase_sta_data();
}
#endif /*SMARTCONFIG_USE_KEY_DOWN*/

void rda5991h_smartlink_irq_init()
{
    //printf("%s\r\n", __func__);
#ifdef SMARTCONFIG_USE_KEY_DOWN
    smartconfig_key_down.fall(&wland_smartlink_irq_fall);
    //smartconfig_key_down.rise(&wland_smartlink_irq_rise);
    timer.start();
#else /*SMARTCONFIG_USE_KEY_DOWN*/
    //smartconfig_key.rise(&wland_smartlink_irq);
    smartconfig_key.fall(&wland_smartlink_irq);
#endif /*SMARTCONFIG_USE_KEY_DOWN*/
}

void do_recv_thread(void *argument)
{
    unsigned int index, size, res;
    char recv_buf[RECV_LIMIT+1];
    index = *(int *)argument;
    while(1){
        if(rda_socket[index].type == TCP){
            size = ((TCPSocket*)(rda_socket[index].socket))->recv((void *)recv_buf, RECV_LIMIT);
            RDA_AT_PRINT("do_recv_thread size %d %d\r\n", size, index);
            if(size <= 0){
                ((TCPSocket*)(rda_socket[index].socket))->close();
                delete (TCPSocket*)(rda_socket[index].socket);
                if(trans_index == index){
                    do{
                        TCPSocket* tcpsocket = new TCPSocket(&wifi);
                        rda_socket[index].socket = (void *)tcpsocket;
                        res = tcpsocket->connect(rda_socket[index].address);
                    }while(res != 0);
                }else{
                    memset(&rda_socket[index], 0, sizeof(rda_socket_t));
                    RESP("+LINKDOWN=%d\r\n", index);
                    return;
                }
            }
        }else if(rda_socket[index].type == UDP){
            size = ((UDPSocket*)(rda_socket[index].socket))->recvfrom(&rda_socket[index].address, (void *)recv_buf, RECV_LIMIT);
        }
        recv_buf[size] = 0;
        if(trans_index == 0xff){
            RESP("+IPD=%d,%d,%s,%d,", index, size, rda_socket[index].SERVER_ADDR, rda_socket[index].SERVER_PORT);
            RESP("%s\r\n",recv_buf);
        }else if(trans_index == index){
            RESP("%s\r\n",recv_buf);
        }else{
            continue;
        }
    }
}

int do_ndns(cmd_tbl_t *cmd, int argc, char *argv[])
{
    ip_addr_t addr;
    int ret;
    if(argc<2){
        RESP_ERROR(ERROR_ARG);
        return 0;
    }

    if(conn_flag == 0){
        RESP_ERROR(ERROR_ABORT);
        return 0;
    }
    ret = netconn_gethostbyname(argv[1], &addr);
    if(ret != 0){
        RESP_ERROR(ERROR_FAILE);
        return 0;
    }

    RESP_OK_EQU("%s\r\n", inet_ntoa(addr));

    return 0;
}


int do_nmode(cmd_tbl_t *cmd, int argc, char *argv[])
{
    int last, size, index;
    unsigned int cnt = 0, time = os_time, tmp_len;
    unsigned char send_buf[SEND_LIMIT+1];
    int ret;

    if(argc<2){
        RDA_AT_PRINT("Please set socket index!\r\n");
        RESP_ERROR(ERROR_ARG);
        return 0;
    }

    index = atoi(argv[1]);

    if(rda_socket[index].used != 1){
        RDA_AT_PRINT("Socket not in used!\r\n");
        RESP_ERROR(ERROR_ABORT);
        return 0;
    }

    RDA_AT_PRINT("enter transparent transmission mode!\r\n");
    //rda_thread_new("tcp_send", do_recv_thread, (void *)&index, 1024*4, osPriorityNormal, &thread_recv);
    trans_index = index;
    while(1) {
        tmp_len = console_fifo_get(&send_buf[cnt], SEND_LIMIT-cnt);
        if(cnt==0 && tmp_len != 0){
            cnt += tmp_len;
            if(send_buf[0] == '+'){
                time = os_time;
                do{
                    tmp_len = console_fifo_get(&send_buf[cnt], SEND_LIMIT-cnt);
                    cnt += tmp_len;
                }while(cnt<3 && os_time-time < 200);

                if(cnt==3 && send_buf[1]=='+' && send_buf[2]=='+'){
                    osDelay(20);
                    if(!console_fifo_len()){
                        console_putc('a');
                        time = os_time;
                        do{
                            tmp_len = console_fifo_get(&send_buf[cnt], SEND_LIMIT-cnt);
                            cnt += tmp_len;
                        }while(tmp_len == 0 && os_time-time<3000);
                        if(cnt==4 && send_buf[3]=='a'){
                            osDelay(20);
                            if(!console_fifo_len()){
 #if 1
                                if(rda_socket[index].type == TCP){
                                    size = ((TCPSocket*)(rda_socket[index].socket))->close();
                                    delete (TCPSocket*)(rda_socket[index].socket);
                                }else if(rda_socket[index].type == UDP){
                                    size = ((UDPSocket*)(rda_socket[index].socket))->close();
                                    delete (UDPSocket*)(rda_socket[index].socket);
                                }
                                ret = rda_thread_delete(rda_socket[index].threadid);
                                RDA_AT_PRINT("exit send transparent transmition! status = %d\r\n", ret);
                                memset(&rda_socket[index], 0, sizeof(rda_socket_t));
 #endif
                                trans_index = 0xff;
                                return 0;
                            }
                        }
                    }
                }
            }
        }else{
            cnt += tmp_len;
        }

        if(cnt > 315 || (os_time - time > 100 && cnt > 0) ) {
            time = os_time;
            if(rda_socket[index].type == TCP){
                do{
                    size = ((TCPSocket*)(rda_socket[index].socket))->send((void *)send_buf, cnt);
                    }while(size == NSAPI_ERROR_NO_MEMORY);
                cnt = 0;
                RDA_AT_PRINT("tcp send size %d\r\n", size);
            }else if(rda_socket[index].type == UDP){
                size = ((UDPSocket*)(rda_socket[index].socket))->sendto(rda_socket[index].SERVER_ADDR, \
                    rda_socket[index].SERVER_PORT, (void *)send_buf, cnt);
                cnt = 0;
                RDA_AT_PRINT("udp send size %d\r\n", size);
            }
        }
        last = console_fifo_len();
        if(last >= SEND_LIMIT/8)
            continue;
        osThreadYield();
    }
}
#if 0
 int do_nrecv(cmd_tbl_t *cmd, int argc, char *argv[])
 {

    unsigned int index, total_len, size;
    char *buf;

    if(argc<3){
        RDA_AT_PRINT("Please set socket index!\r\n");
        RESP_ERROR(ERROR_ARG);
        return 0;
    }

    index = atoi(argv[1]);
    total_len = atoi(argv[2]);

    if(rda_socket[index].used != 1){
        RDA_AT_PRINT("Socket not in used!\r\n");
        RESP_ERROR(ERROR_ABORT);
    }

    if(total_len > RECV_LIMIT){
        RDA_AT_PRINT("Send data len longger than %d!\r\n", SEND_LIMIT);
        RESP_ERROR(ERROR_ARG);
    }

    buf = (char *)malloc(total_len+1);

    if(rda_socket[index].type == TCP){
        size = ((TCPSocket*)(rda_socket[index].socket))->recv((void *)buf, total_len);
        RDA_AT_PRINT("recv tcp size %d!\r\n", size);
    }else if(rda_socket[index].type == UDP){
        size = ((UDPSocket*)(rda_socket[index].socket))->recvfrom(&rda_socket[index].address, (void *)buf, total_len);
        RDA_AT_PRINT("recv udp size %d!\r\n", size);
    }
    if(size > 0){
        buf[size] = 0;
        RESP_OK_EQU("%d,%s\r\n", size, buf);
    }else{
        RESP_ERROR(ERROR_FAILE);
    }
    free(buf);
    return 0;
 }
#endif
int do_nsend(cmd_tbl_t *cmd, int argc, char *argv[])
{
    unsigned int index, total_len, len, tmp_len, size;
    unsigned char *buf;

    if(argc<3){
        RDA_AT_PRINT("Please set socket index!\r\n");
        RESP_ERROR(ERROR_ARG);
        return 0;
    }

    index = atoi(argv[1]);
    total_len = atoi(argv[2]);

    if(rda_socket[index].used != 1){
        RDA_AT_PRINT("Socket not in used!\r\n");
        RESP_ERROR(ERROR_ABORT);
    }

    if(total_len > SEND_LIMIT){
        RDA_AT_PRINT("Send data len longger than %d!\r\n", SEND_LIMIT);
        RESP_ERROR(ERROR_ARG);
    }

    buf = (unsigned char *)malloc(total_len);
    len = 0;

    while(len < total_len) {
        tmp_len = console_fifo_get(&buf[len], total_len-len);
        len += tmp_len;
    }

    if(rda_socket[index].type == TCP){
        size = ((TCPSocket*)(rda_socket[index].socket))->send((void *)buf, total_len);
        RDA_AT_PRINT("tcp send size %d\r\n", size);
    }else if(rda_socket[index].type == UDP){
        size = ((UDPSocket*)(rda_socket[index].socket))->sendto(rda_socket[index].SERVER_ADDR, \
            rda_socket[index].SERVER_PORT, (void *)buf, total_len);
        RDA_AT_PRINT("udp send size %d\r\n", size);
    }

    free(buf);
    if(size == total_len) {
        RESP_OK();
    } else {
        RESP_ERROR(ERROR_FAILE);
    }
    return 0;
}

int do_nstop(cmd_tbl_t *cmd, int argc, char *argv[])
{
    int index;
    if(argc<2){
        RDA_AT_PRINT("Please set socket index!\r\n");
        RESP_ERROR(ERROR_ARG);
        return 0;
    }

    index = atoi(argv[1]);
    if(rda_socket[index].used != 1){
        RDA_AT_PRINT("Socket not in used!\r\n");
        RESP_ERROR(ERROR_ABORT);
    }
    if(rda_socket[index].type == TCP){
        ((TCPSocket*)(rda_socket[index].socket))->close();
    }else if(rda_socket[index].type == UDP){
        ((UDPSocket*)(rda_socket[index].socket))->close();
    }
    delete rda_socket[index].socket;
    rda_thread_delete(rda_socket[index].threadid);
    memset(&rda_socket[index], 0, sizeof(rda_socket_t));
    RESP_OK();
    return 0;
}

int do_nstart(cmd_tbl_t *cmd, int argc, char *argv[])
{
    int res, index;

    if(argc<3){
        RDA_AT_PRINT("Please set server ip and server port!\r\n");
        RESP_ERROR(ERROR_ARG);
        return 0;
    }
    for(index=0; index<SOCKET_NUM; index++)
        if(rda_socket[index].used == 0)
            break;

    if(index == SOCKET_NUM){
        RESP_ERROR(ERROR_FAILE);
        return 0;
    }

    memcpy(rda_socket[index].SERVER_ADDR, argv[2], strlen(argv[2]));
    rda_socket[index].SERVER_PORT = atoi(argv[3]);
    RDA_AT_PRINT("ip %s port %d\r\n", rda_socket[index].SERVER_ADDR, rda_socket[index].SERVER_PORT);
    rda_socket[index].address = SocketAddress(rda_socket[index].SERVER_ADDR, rda_socket[index].SERVER_PORT);

    if(!strcmp(argv[1], "TCP")){
        TCPSocket* tcpsocket = new TCPSocket(&wifi);
        rda_socket[index].type = TCP;
        rda_socket[index].socket = (void *)tcpsocket;
        res = tcpsocket->connect(rda_socket[index].address);
        if(res != 0){
            RDA_AT_PRINT("connect failed res = %d\r\n", res);
            RESP_ERROR(ERROR_FAILE);
            return 0;
        }
    }else if(!strcmp(argv[1], "UDP")){
        UDPSocket* udpsocket = new UDPSocket(&wifi);
        rda_socket[index].type = UDP;
        rda_socket[index].socket = (void *)udpsocket;
        if(argc == 5){
            udpsocket->bind(atoi(argv[4]));
            rda_socket[index].LOCAL_PORT = atoi(argv[4]);
        }
    }else{
        RESP_ERROR(ERROR_ARG);
        memset(&rda_socket[index], 0, sizeof(rda_socket_t));
        return 0;
    }
    rda_socket[index].used = 1;

    rda_socket[index].threadid = rda_thread_new(NULL, do_recv_thread, (void *)&index, 1024*4, osPriorityNormal);
    RESP_OK_EQU("%d\r\n", index);
    return 0;
}

int do_nlink(cmd_tbl_t *cmd, int argc, char *argv[])
{
    int i, n=0;

    for(i=0; i<SOCKET_NUM; i++)
        if(rda_socket[i].used == 1)
            n++;

    RESP_OK_EQU("%d\r\n", n);

    for(i=0; i<SOCKET_NUM; i++){
        if(rda_socket[i].used == 1)
            RESP_OK_EQU("%d,%s,%s,%d,%d\r\n", i, rda_socket[i].type ? "UDP":"TCP", rda_socket[i].SERVER_ADDR, \
                rda_socket[i].SERVER_PORT, rda_socket[i].LOCAL_PORT);
    }

    return 0;
}

int do_nping(cmd_tbl_t *cmd, int argc, char *argv[])
{
    int count, interval, len, echo_level;

    if (argc < 6) {
        RESP_ERROR(ERROR_ARG);
        return 0;
    }

    count = atoi(argv[2]);
    interval = atoi(argv[3]);
    len = atoi(argv[4]);
    echo_level = atoi(argv[5]);
    if (count < 0 || interval < 0 || len < 0) {
        RESP_ERROR(ERROR_ARG);
        return 0;
    }
    if (interval < 1 )
        interval = 1000; // s -> ms
    else if (interval > 10)
        interval = 10 * 1000; // s -> ms
    else
        interval *= 1000; // s -> ms

    if (len > 14600)
        len = 14600;

    if (echo_level > 0)
        echo_level = 1;
    else
        echo_level = 0;

    ping_init((const char*)argv[1], count, interval, len, echo_level);
    return 0;
}

int do_wsfixip(cmd_tbl_t *cmd, int argc, char *argv[])
{
    int enable, flag;
    unsigned int ip, msk, gw;
    if (argc < 3) {
        RESP_ERROR(ERROR_ARG);
        return 0;
    }

    if(conn_flag == 1){
        RESP_ERROR(ERROR_ABORT);
        return 0;
    }

    flag = atoi(argv[1]);
    enable = atoi(argv[2]);

    if(flag == 0){
        fixip_flag = 1;
        if(enable == 0){
            wifi.set_dhcp(1);
            wifi.set_network(NULL, NULL, NULL);
        }else if(enable == 1){
            wifi.set_dhcp(0);
            wifi.set_network(argv[3], argv[4], argv[5]);
        }
    }else if(flag == 1){
        if(enable == 0){
            rda5981_flash_write_dhcp_data(0, 0, 0, 0);
        }else if(enable == 1){
            ip = inet_addr(argv[3]);
            msk = inet_addr(argv[4]);
            gw = inet_addr(argv[5]);
            rda5981_flash_write_dhcp_data(1, ip, msk, gw);
        }
    }
    RESP_OK();
    return 0;
}

int do_wsscan(cmd_tbl_t *cmd, int argc, char *argv[])
{
    int cnt=0, num=0, i;
    rda5981_scan_result *bss_list = NULL;

    cnt = wifi.scan(NULL, 0);

    if(*argv[1] == '?'){
        bss_list = (rda5981_scan_result *)malloc(cnt * sizeof(rda5981_scan_result));
        memset(bss_list, 0, cnt * sizeof(rda5981_scan_result));
        num = wifi.scan_result(bss_list, cnt);
        RESP_OK_EQU("%d\r\n", num);
        for(i=0; i<num; i++){
            RESP_OK_EQU("%02x:%02x:%02x:%02x:%02x:%02x,", bss_list[i].BSSID[0], bss_list[i].BSSID[1], bss_list[i].BSSID[2], bss_list[i].BSSID[3], bss_list[i].BSSID[4], bss_list[i].BSSID[5]);
            RESP("%d,%d,%s\r\n", bss_list[i].channel, bss_list[i].RSSI, bss_list[i].SSID);
        }
        free(bss_list);
    }else{
        RESP_OK();
    }

    return 0;
}

int do_wapstop( cmd_tbl_t *cmd, int argc, char *argv[])
{
    if(ap_flag == 0){
        RESP_ERROR(ERROR_ABORT);
        return 0;
    }
    wifi.stop_ap(0);
    ap_flag = 0;

    RESP_OK();
    return 0;
}

int do_wap( cmd_tbl_t *cmd, int argc, char *argv[])
{
    unsigned char channel = 0;
    unsigned int ip, msk, gw, dhcps, dhcpe, flag;
    char ips[IP_LENGTH], msks[IP_LENGTH], gws[IP_LENGTH], dhcpss[IP_LENGTH], dhcpes[IP_LENGTH];
    if (argc < 1) {
        RESP_ERROR(ERROR_ARG);
        return 0;
    }

    if(*argv[1] == '?'){
        if(ap_flag == 1){
            RESP_OK_EQU("ssid:%s, pw:%s, ip:%s, BSSID:%s\r\n", ssid_ap, pw_ap, wifi.get_ip_address_ap(), wifi.get_mac_address_ap());
        }else{
            RESP_OK_EQU("ssid:, pw:, ip:0.0.0.0, BSSID:00:00:00:00:00:00\r\n");
        }
        return 0;
    }

    if(ap_flag == 1){
        RESP_ERROR(ERROR_ABORT);
        return 0;
    }

    memset(ssid_ap, 0, sizeof(ssid_ap));
    memset(pw_ap, 0, sizeof(pw_ap));

    if(argc == 1){
        if(rda5981_flash_read_ap_data(ssid_ap, pw_ap, &channel)){
            RESP_ERROR(ERROR_FAILE);
            return 0;
        }
    }else{
        flag = atoi(argv[1]);
        channel = atoi(argv[2]);
        if(channel <1 || channel > 13)
            channel = 6;

        memcpy(ssid_ap, argv[3], strlen(argv[3]));

        if(argc > 3)
            memcpy(pw_ap, argv[4], strlen(argv[4]));
    }

    if(ap_net_flag == 0){
        if(!rda5981_flash_read_ap_net_data(&ip, &msk, &gw, &dhcps, &dhcpe)){
            memcpy(ips, inet_ntoa(ip), IP_LENGTH);
            memcpy(msks, inet_ntoa(msk), IP_LENGTH);
            memcpy(gws, inet_ntoa(gw), IP_LENGTH);
            memcpy(dhcpss, inet_ntoa(dhcps), IP_LENGTH);
            memcpy(dhcpes, inet_ntoa(dhcpe), IP_LENGTH);
            wifi.set_network_ap(ips, msks, gws, dhcpss, dhcpes);
        }
    }
    ap_net_flag = 0;
    wifi.start_ap(ssid_ap, pw_ap, channel);
    Thread::wait(2000);
    if(flag == 1)
        rda5981_flash_write_ap_data(ssid_ap, pw_ap, channel);

    ap_flag = 1;
    RESP_OK();

    return 0;
}

int conn(char *ssid, char *pw, char *bssid)
{
    int ret;
    const char *ip_addr;
    unsigned int enable, ip, msk, gw;
    char ips[IP_LENGTH], msks[IP_LENGTH], gws[IP_LENGTH];

    if(fixip_flag == 0){
        ret = rda5981_flash_read_dhcp_data(&enable, &ip, &msk, &gw);
        if(ret == 0 && enable == 1){
            memcpy(ips, inet_ntoa(ip), IP_LENGTH);
            memcpy(msks, inet_ntoa(msk), IP_LENGTH);
            memcpy(gws, inet_ntoa(gw), IP_LENGTH);
            wifi.set_dhcp(0);
            wifi.set_network(ips, msks, gws);
        }else{
            wifi.set_dhcp(1);
        }
    }
    fixip_flag = 0;
    ret = wifi.connect(ssid, pw, bssid, NSAPI_SECURITY_NONE, 0);
    if(ret != 0){
        //RESP_ERROR(ERROR_FAILE);
        return -1;
    }

    ip_addr = wifi.get_ip_address();

    if (ip_addr) {
        RDA_AT_PRINT("Client IP Address is %s\r\n", ip_addr);
        conn_flag = 1;
        return 0;
    } else {
        RDA_AT_PRINT("No Client IP Address\r\n");
        return -1;
    }
}

int do_wsc( cmd_tbl_t *cmd, int argc, char *argv[])
{
    int ret, flag;
    const char *ip_addr, *mac_addr;

    if (argc < 2) {
        RESP_ERROR(ERROR_ARG);
        return 0;
    }

    if(conn_flag == 1){
        RESP_ERROR(ERROR_ABORT);
        RDA_AT_PRINT("error! Has been connected!");
        return 0;
    }

    wifi.scan(NULL, 0);//necessary
    ret = rda5981_getssid_from_smartconfig(ssid, pw);
    if (ret) {
        RDA_AT_PRINT("smartconfig fail, ret: %d \r\n", ret);
        RESP_ERROR(ERROR_FAILE);
        return 0;
    } else {
        RDA_AT_PRINT("smartconfig success, ssid = \"%s\", pw = \"%s\" \r\n", ssid, pw);
    }

    flag = atoi(argv[1]);
    ret = conn(ssid, pw, NULL);
    if(ret == 0){
        if(flag == 1)
            rda5981_flash_write_sta_data(ssid, pw);
        ip_addr = wifi.get_ip_address();
        mac_addr = wifi.get_mac_address();
        rda5981_sendrsp_to_host(ip_addr, mac_addr, (void *)&wifi);
        RESP_OK();
    }else{
        RESP_ERROR(ERROR_FAILE);
    }
    return 0;
}

int do_wsak( cmd_tbl_t *cmd, int argc, char *argv[])
{
    int ret, flag;
    const char *ip_addr, *mac_addr;

    if (argc < 2) {
        RESP_ERROR(ERROR_ARG);
        return 0;
    }

    if(conn_flag == 1){
        RESP_ERROR(ERROR_ABORT);
        RDA_AT_PRINT("error! Has been connected!");
        return 0;
    }

    ret = get_ssid_pw_from_airkiss(ssid, pw);
    if (ret) {
        RDA_AT_PRINT("smartconfig fail, ret: %d \r\n", ret);
        RESP_ERROR(ERROR_FAILE);
        return 0;
    } else {
        RDA_AT_PRINT("smartconfig success, ssid = \"%s\", pw = \"%s\" \r\n", ssid, pw);
    }

    flag = atoi(argv[1]);
    ret = conn(ssid, pw, NULL);
    if(ret == 0){
        if(flag == 1)
            rda5981_flash_write_sta_data(ssid, pw);
        ip_addr = wifi.get_ip_address();
        mac_addr = wifi.get_mac_address();
        airkiss_sendrsp_to_host((void *)&wifi);
        RESP_OK();
    }else{
        RESP_ERROR(ERROR_FAILE);
    }
    return 0;
}

int do_wsconn( cmd_tbl_t *cmd, int argc, char *argv[])
{
    int ret, flag;

    if (argc < 1) {
        RESP_ERROR(ERROR_ARG);
        return 0;
    }

    if(*argv[1] == '?'){
        if(conn_flag == 1){
            wifi.update_rssi();
            RESP_OK_EQU("ssid:%s, RSSI:%ddb, ip:%s, BSSID: "MAC_FORMAT"\r\n", wifi.get_ssid(), wifi.get_rssi(), wifi.get_ip_address(), MAC2STR(wifi.get_BSSID()));
        }else{
            RESP_OK_EQU("ssid:, RSSI:0db, ip:0.0.0.0\r\n");
        }
        return 0;
    }

    if(conn_flag == 1){
        RESP_ERROR(ERROR_ABORT);
        RDA_AT_PRINT("error! Has been connected!");
        return 0;
    }

    if(argc == 1)
        flag = 0;
    else
        flag = atoi(argv[1]);

    memset(ssid, 0, sizeof(ssid));
    memset(pw, 0, sizeof(pw));

    if(argc > 2)
        memcpy(ssid, argv[2], strlen(argv[2]));

    if(argc > 3)
        memcpy(pw, argv[3], strlen(argv[3]));

    if (strlen(ssid) == 0) {
        ret = rda5981_flash_read_sta_data(ssid, pw);
        if (ret == 0 && strlen(ssid)) {
            RDA_AT_PRINT("get ssid from flash: ssid:%s, pass:%s\r\n", ssid, pw);
        }else{
            RESP_ERROR(ERROR_ARG);
            return 0;
        }
    }
    RDA_AT_PRINT("ssid %s pw %s\r\n", ssid, pw);

    ret = conn(ssid, pw, NULL);
    if(ret == 0){
        if(flag == 1)
            rda5981_flash_write_sta_data(ssid, pw);
        RESP_OK();
    }else{
        RESP_ERROR(ERROR_FAILE);
    }
    return 0;
}

int do_disconn( cmd_tbl_t *cmd, int argc, char *argv[])
{
    if(conn_flag == 0){
        RESP_ERROR(ERROR_ABORT);
        return 0;
    }

    wifi.disconnect();
    while(wifi.get_ip_address() != NULL)
        Thread::wait(20);
    conn_flag = 0;
    RESP_OK();
    return 0;
}

int do_dbg( cmd_tbl_t *cmd, int argc, char *argv[])
{
    int arg;

    if (argc < 3) {
        RESP_ERROR(ERROR_ARG);
        return 0;
    }

    arg = atoi(argv[2]);
    if(!strcmp(argv[1], "DRV")){
        wland_dbg_level = arg;
    }else if(!strcmp(argv[1], "WPA")){
        wpa_dbg_level = arg;
    }else if(!strcmp(argv[1], "DRVD")){
        wland_dbg_dump = arg;
    }else if(!strcmp(argv[1], "WPAD")){
        wpa_dbg_dump = arg;
    }else{
        RESP_ERROR(ERROR_ARG);
        return 0;
    }

    RESP_OK();

    return 0;
}

int do_uart( cmd_tbl_t *cmd, int argc, char *argv[])
{

    if (argc < 2) {
        RESP_ERROR(ERROR_ARG);
        return 0;
    }

    //inquire
    if(*argv[1] == '?'){
        RESP_OK_EQU("%d\r\n", baudrate);
        return 0;
    }

    baudrate = atoi(argv[2]);
    console_set_baudrate(baudrate);
    if(atoi(argv[1]) == 1)
        rda5981_flash_write_uart(&baudrate);
    RESP_OK();

    return 0;
}

int do_wsmac( cmd_tbl_t *cmd, int argc, char *argv[])
{
    char *mdata, mac[6], i;

    if(*argv[1] == '?'){
        rda_get_macaddr((unsigned char *)mac, 0);
        RESP_OK_EQU("%02x:%02x:%02x:%02x:%02x:%02x\r\n",
            mac[0], mac[1], mac[2], mac[3], mac[4], mac[5]);
        return 0;
    }

    if(conn_flag == 1 || ap_flag == 1){
        RESP_ERROR(ERROR_ABORT);
        return 0;
    }

    if(strlen(argv[1]) != 12){
        RESP_ERROR(ERROR_ARG);
        return 0;
    }

    mdata = argv[1];
    for(i = 0; i < 12; i++){
        if(mdata[i] >= 0x41 && mdata[i] <= 0x46)// switch 'A' to 'a'
            mdata[i] += 0x20;
        if(mdata[i] >= 0x61 && mdata[i] <= 0x66)//switch "ab" to 0xab
            mac[i] = mdata[i] - 0x57;
        if(mdata[i] >= 0x30 && mdata[i] <= 0x39)
            mac[i] = mdata[i] - 0x30;
            if(i%2 == 1)
                mac[i/2] = mac[i-1] << 4 | mac[i];
    }
    if(!mac_is_valid(mac)){
        RESP_ERROR(ERROR_ARG);
        return 0;
    }
    memcpy(rda_mac_addr, mac, 6);
    rda5981_flash_write_mac_addr(rda_mac_addr);
    RESP_OK();

    return 0;
}

int do_wamac( cmd_tbl_t *cmd, int argc, char *argv[])
{
    char *mdata, mac[6], i;

    if(*argv[1] == '?'){
        rda_get_macaddr((unsigned char *)mac, 1);
        RESP_OK_EQU("%02x:%02x:%02x:%02x:%02x:%02x\r\n",
            mac[0], mac[1], mac[2], mac[3], mac[4], mac[5]);
        return 0;
    }
    RESP_ERROR(ERROR_ARG);
/*
    if(strlen(argv[1]) != 12){
        RESP_ERROR(ERROR_ARG);
        return 0;
    }

    mdata = argv[1];
    for(i = 0; i < 12; i++){
        if(mdata[i] >= 0x41 && mdata[i] <= 0x46)// switch 'A' to 'a'
            mdata[i] += 0x20;
        if(mdata[i] >= 0x61 && mdata[i] <= 0x66)//switch "ab" to 0xab
            mac[i] = mdata[i] - 0x57;
        if(mdata[i] >= 0x30 && mdata[i] <= 0x39)
            mac[i] = mdata[i] - 0x30;
            if(i%2 == 1)
                mac[i/2] = mac[i-1] << 4 | mac[i];
    }
    if(!mac_is_valid(mac)){
        RESP_ERROR(ERROR_ARG);
        return 0;
    }

    if(mac[0] & 0x04)
        mac[0] &= 0xFB;
    else
        mac[0] |= 0x04;

    memcpy(rda_mac_addr, mac, 6);
    rda5981_flash_write_mac_addr(rda_mac_addr);
    RESP_OK();
*/
    return 0;
}

int do_wasta( cmd_tbl_t *cmd, int argc, char *argv[])
{
    int cnt = 5, num, i;
    rda5981_apsta_info * sta_list;

    sta_list = (rda5981_apsta_info *)malloc(5 * sizeof(rda5981_apsta_info));
    memset(sta_list, 0, cnt * sizeof(rda5981_apsta_info));
    num = wifi.ap_join_info(sta_list, cnt);
    RESP_OK_EQU("%d\r\n", num);
    for(i=0; i<num; i++){
        RESP_OK_EQU("%02x:%02x:%02x:%02x:%02x:%02x,", sta_list[i].mac[0], sta_list[i].mac[1], sta_list[i].mac[2], sta_list[i].mac[3], sta_list[i].mac[4], sta_list[i].mac[5]);
        RESP("%s\r\n", inet_ntoa(sta_list[i].ip));
    }
    free(sta_list);
    return 0;
}

int do_wanet(cmd_tbl_t *cmd, int argc, char *argv[])
{
    unsigned int ip, msk, gw, dhcps, dhcpe, flag, ret;

    if (argc < 1) {
        RESP_ERROR(ERROR_ARG);
        return 0;
    }

    if(*argv[1] == '?'){
        if(ap_net_flag == 1 || ap_flag == 1){
            RESP_OK_EQU("IP:%s,", wifi.get_ip_address_ap());
            RESP("MASK:%s,", wifi.get_netmask_ap());
            RESP("GW:%s,", wifi.get_gateway_ap());
            RESP("DHCPS:%s,", wifi.get_dhcp_start_ap());
            RESP("DHCPE:%s\r\n", wifi.get_dhcp_end_ap());
            return 0;
        }

        ret = rda5981_flash_read_ap_net_data(&ip, &msk, &gw, &dhcps, &dhcpe);
        if(ret == 0){
            RESP_OK_EQU("IP:%s,", inet_ntoa(ip));
            RESP("MASK:%s,",inet_ntoa(msk));
            RESP("GW:%s,",inet_ntoa(gw));
            RESP("DHCPS:%s,",inet_ntoa(dhcps));
            RESP("DHCPE:%s\r\n",inet_ntoa(dhcpe));
        }else{
            RESP_OK_EQU("IP:%s,", inet_ntoa(*(unsigned int *)mbed_ip_addr));
            RESP("MASK:%s,",inet_ntoa(*(unsigned int *)mbed_msk_addr));
            RESP("GW:%s,",inet_ntoa(*(unsigned int *)mbed_gw_addr));
            RESP("DHCPS:%s,",inet_ntoa(*(unsigned int *)mbed_dhcp_start));
            RESP("DHCPE:%s\r\n",inet_ntoa(*(unsigned int *)mbed_dhcp_end));
        }
        return 0;
    }

    if(ap_flag == 1){
        RESP_ERROR(ERROR_ABORT);
        return 0;
    }

    flag = atoi(argv[1]);
    if(flag == 0){
        wifi.set_network_ap(argv[2], argv[3], argv[4], argv[5], argv[6]);
        ap_net_flag = 1;
    }if(flag == 1){
        ip = inet_addr(argv[2]);
        msk = inet_addr(argv[3]);
        gw = inet_addr(argv[4]);
        dhcps = inet_addr(argv[5]);
        dhcpe = inet_addr(argv[6]);
        rda5981_flash_write_ap_net_data(ip, msk, gw, dhcps, dhcpe);
    }

    RESP_OK();
    return 0;
}

int do_ver( cmd_tbl_t *cmd, int argc, char *argv[])
{
    RESP_OK_EQU("%s\r\n", version);
    return 0;
}

int do_userdata( cmd_tbl_t *cmd, int argc, char *argv[])
{
    int len, ret, tmp_len = 0;
    unsigned char *buf;

    if (argc < 2) {
        RESP_ERROR(ERROR_ARG);
        return 0;
    }

    if(*argv[1] == '?'){
        len  = rda5981_flash_read_3rdparter_data_length();
        if (len < 0) {
            RDA_AT_PRINT("No user data in flah\r\n");
            RESP_ERROR(ERROR_FAILE);
            return 0;
        }

        buf = (unsigned char *)malloc(len+1);
        memset(buf, 0, len+1);

        ret = rda5981_flash_read_3rdparter_data(buf, len);
        if(ret < 0)
            RDA_AT_PRINT("read flash error, error %d\r\n", ret);
        else
            RDA_AT_PRINT("Data read complete, len:%d\r\n", ret);

        buf[len] = 0;
        RESP_OK_EQU("%d,%s\r\n", len, buf);
        free(buf);
        return 0;
    }

    len = atoi(argv[1]);
    buf = (unsigned char *)malloc(len);
    memset(buf, 0, len);

    do{
        unsigned int size;
        size = console_fifo_get(buf+tmp_len, len-tmp_len);
        tmp_len += size;
    }while(tmp_len < len);

    ret = rda5981_flash_write_3rdparter_data(buf, len);
    if(ret < 0){
        RDA_AT_PRINT("write flash error, error %d\r\n", ret);
        RESP_ERROR(ERROR_FAILE);
    }else{
        RDA_AT_PRINT("Data write complete\r\n");
        RESP_OK();
    }

    free(buf);
    return 0;
}

int do_at( cmd_tbl_t *cmd, int argc, char *argv[])
{
    RESP_OK();
    return 0;
}

int do_rst( cmd_tbl_t *cmd, int argc, char *argv[])
{
    RESP_OK();
    rda_wdt_softreset();
    return 0;

}

int do_echo( cmd_tbl_t *cmd, int argc, char *argv[])
{
    int flag;

    if (argc < 2) {
        RESP_ERROR(ERROR_ARG);
        return 0;
    }
    flag = atoi(argv[1]);

    if(flag != 0 && flag != 1){
        RESP_ERROR(ERROR_ARG);
        return 0;
    }

    echo_flag = flag;
    RESP_OK();
    return 0;

}

int do_restore( cmd_tbl_t *cmd, int argc, char *argv[])
{
    if(argc < 1) {
        RESP_ERROR(ERROR_ARG);
        return 0;
    }

    rda5981_flash_erase_ap_data();
    rda5981_flash_erase_ap_net_data();
    rda5981_flash_erase_dhcp_data();
    rda5981_flash_erase_sta_data();
    rda5981_flash_erase_uart();

    RESP_OK();
    return 0;
}


int do_h( cmd_tbl_t *cmd, int argc, char *argv[])
{
    cmd_tbl_t *cmdtemp = NULL;
    int i;

    for (i = 0; i<(int)cmd_cntr; i++) {
        cmdtemp = &cmd_list[i];
        if(cmdtemp->usage) {
            printf("%s\r\n", cmdtemp->usage);
        }
    }
    return 0;
}

#if DEVICE_SLEEP
int do_sleep( cmd_tbl_t *cmd, int argc, char *argv[])
{
    int enable;
    if(argc < 2) {
        RESP_ERROR(ERROR_ARG);
        return 0;
    }
    enable = atoi(argv[1]);
    if(0 == enable) {
        sleep_set_level(SLEEP_NORMAL);
        RESP_OK();
    } else if(1 == enable) {
        sleep_set_level(SLEEP_DEEP);
        RESP_OK();
    } else {
        RESP_ERROR(ERROR_ARG);
    }
    return 0;
}

int do_user_sleep( cmd_tbl_t *cmd, int argc, char *argv[])
{
    if(argc < 1) {
        RESP_ERROR(ERROR_ARG);
        return 0;
    }
    user_sleep_allow(1);
    RESP_OK();
    return 0;
}

InterruptIn wakeup_key(PB_6);
void donothing(void) {}

int do_wake_source(cmd_tbl_t *cmd, int argc, char *argv[])
{
    int src, enable;
    if(argc < 3) {
        RESP_ERROR(ERROR_ARG);
        return -1;
    }
    src = atoi(argv[1]);
    enable = atoi(argv[2]);
    if(0 == src) { // uart
        user_sleep_wakesrc_set(WAKESRC_UART, enable);
    } else if(1 == src) { // gpio
        int gp_num = 0;
        if(enable) {
            if(argc < 4) {
                RDA_AT_PRINT("WrongArgCnt:%d\r\n", argc);
                return -1;
            }
            gp_num = atoi(argv[3]);
            if((6 <= gp_num) && (gp_num <= 9)) {
                rda_ccfg_gp(gp_num, 1);
            }
            wakeup_key.fall(&donothing);
            wakeup_key.rise(&donothing);
        }
        user_sleep_wakesrc_set(WAKESRC_GPIO, (unsigned int)((enable & 0xFFUL) | (gp_num << 8)));
    }
    RESP_OK();
    return 0;
}
#endif

int do_bootaddr(cmd_tbl_t *cmd, int argc, char *argv[])
{
    int ret;
    unsigned int reboot_addr;

    if(argc < 2) {
        RESP_ERROR(ERROR_ARG);
        return 0;
    }
    reboot_addr = strtol(argv[1], NULL, 0);
    RDA_AT_PRINT("boot to addr:0x%x\n", reboot_addr);
    ret = rda5981_reboot_to_addr(RDA5981_FIRMWARE_INFO_ADDR, reboot_addr);
    if (ret) {
        RDA_AT_PRINT("reboot to addr :0x%x fail\n", reboot_addr);
        RESP_ERROR(ERROR_FAILE);
    } else
        RESP_OK();
    return 0;
}

int do_rw_fatfile(cmd_tbl_t *cmd, int argc, char *argv[])
{
    const char *file_name0 = "/sdc/mydir/sdctest.txt";
    const char *wrstr = "Hello world!\r\nSD Card\r\n";
    char rdstr[64] = {'\0'};
    FILE *fp = NULL;
    int err = 0;

    if(argc < 1) {
        RESP_ERROR(ERROR_ARG);
        return 0;
    }

    RDA_AT_PRINT("Start SD Card Test...\r\n\r\n");

    err = sdc.disk_check();
    if(0 != err) {
        RDA_AT_PRINT("Could not find Sd Card!\r\n");
        return -1;
    }

    mkdir("/sdc/mydir", 0777);

    fp = fopen(file_name0, "w+");
    if(NULL == fp) {
        RDA_AT_PRINT("Could not open file for write!\r\n");
        return -1;
    }

    fprintf(fp, wrstr);
    RDA_AT_PRINT("Write text to file: %s\r\n%s\r\n", file_name0, wrstr);

    /* Set fp at file head */
    fseek(fp, 0L, SEEK_SET);

    fread(rdstr, sizeof(char), 64, fp);
    RDA_AT_PRINT("Read text from file: %s\r\n%s\r\n", file_name0, rdstr);
    fclose(fp);

    RDA_AT_PRINT("End of SD Card Test...\r\n");

    RESP_OK();
    return 0;
}
void add_cmd()
{
    int i, j;
    cmd_tbl_t cmd_list[] = {
        /*Basic CMD*/
        {
            "AT+H",             1,   do_h,
            "AT+H               - check AT help"
        },
        {
            "AT+ECHO",          2,   do_echo,
            "AT+ECHO            - open/close uart echo"
        },
        {
            "AT+RST",           1,   do_rst,
            "AT+RST             - Software Reset"
        },
        {
            "AT+VER",           1,   do_ver,
            "AT+VER             - get version"
        },
        {
            "AT+UART",          3,   do_uart,
            "AT+UART            - set/get serial baudrate"
        },
        {
            "AT+WDBG",          3,   do_dbg,
            "AT+WDBG            - adjust debug level"
        },
        {
            "AT+USERDATA",      2,   do_userdata,
            "AT+USERDATA        - write/read user data"
        },
#if DEVICE_SLEEP
        {
            "AT+SLEEP",         2,   do_sleep,
            "AT+SLEEP           - enable/disable sleep"
        },
        {
            "AT+USERSLEEP",     1,   do_user_sleep,
            "AT+USERSLEEP       - set user sleep"
        },
        {
            "AT+WAKESRC",       4,   do_wake_source,
            "AT+WAKESRC         - set wakeup source"
        },
#endif
        {
            "AT+RWFILE",        1,   do_rw_fatfile,
            "AT+RWFILE          - read/write FAT file"
        },
        {
            "AT+RESTORE",       1,   do_restore,
            "AT+RESTORE         - restore default config"
        },
        {
            "AT+BOOTADDR",      2,   do_bootaddr,
            "AT+BOOTADDR        - do bootaddr"
        },
        /*WIFI CMD*/
        {
            "AT+WSMAC",         2,   do_wsmac,
            "AT+WSMAC           - set/get mac address"
        },
        {
            "AT+WSSCAN",        2,   do_wsscan,
            "AT+WSSCAN          - scan AP"
        },
        {
            "AT+WSCONN",        4,   do_wsconn,
            "AT+WSCONN          - start wifi connect"
        },
        {
            "AT+WSDISCONN",     1,   do_disconn,
            "AT+WSDISCONN       - disconnect"
        },
        {
            "AT+WSC",           2,   do_wsc,
            "AT+WSC             - start smart config"
        },
        {
            "AT+WSFIXIP",       6,   do_wsfixip,
            "AT+WSFIXIP         - enable/disable DHCP"
        },
        {
            "AT+WAP",           5,   do_wap,
            "AT+WAP             - enable AP"
        },
        {
            "AT+WAPSTOP",       3,   do_wapstop,
            "AT+WAPSTOP         - stop AP"
        },
        {
            "AT+WAMAC",         2,   do_wamac,
            "AT+WAMAC           - set/get softap mac address"
        },
        {
            "AT+WSAK",          2,   do_wsak,
            "AT+WSSAK           - start wechat airkiss"
        },
        {
            "AT+WASTA",         1,   do_wasta,
            "AT+WASTA           - get joined sta info"
        },
        {
            "AT+WANET",         7,   do_wanet,
            "AT+WANET           - set/get AP net info"
        },
        /*NET CMD*/
        {
            "AT+NSTART",        5,   do_nstart,
            "AT+NSTART          - start tcp/udp client"
        },
        {
            "AT+NSTOP",         2,   do_nstop,
            "AT+NSTOP           - stop tcp/udp client"
        },
        {
            "AT+NSEND",         3,   do_nsend,
            "AT+NSEND           - send tcp/udp data"
        },
#if 0
        {
            "AT+NRECV",         3,   do_nrecv,
            "AT+NRECV           - recv tcp/udp data"
        },
#endif
        {
            "AT+NMODE",         2,   do_nmode,
            "AT+NMODE           - start transparent transmission mode"
        },
        {
            "AT+NLINK",         1,   do_nlink,
            "AT+NLINK           - check tcp/udp client status"
        },
        {
            "AT+NPING",         6,   do_nping,
            "AT+NPING           - do ping"
        },
        {
            "AT+NDNS",          2,   do_ndns,
            "AT+NDNS            - do dns"
        },
    };
    j = sizeof(cmd_list)/sizeof(cmd_tbl_t);
    for(i=0; i<j; i++){
        if(0 != console_cmd_add(&cmd_list[i])) {
            RDA_AT_PRINT("Add cmd failed\r\n");
        }
    }
}

void start_at(void)
{
    console_init();
    user_sleep_init();
    rda5991h_smartlink_irq_init();
    add_cmd();
}

