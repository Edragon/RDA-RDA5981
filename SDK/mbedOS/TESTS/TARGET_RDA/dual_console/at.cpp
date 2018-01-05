#include <stdio.h>
#include "mbed.h"
#include "rtos.h"
#include "console.h"
#include "WiFiStackInterface.h"
#include "rda_wdt_api.h"
#include "rda_sleep_api.h"
#include "Thread.h"
#include "cmsis_os.h"
#include "gpadckey.h"
#include "at.h"
#include "ping/ping.h"
#include "smart_config.h"
#include "inet.h"
#include "lwip/api.h"
#include "rda_sys_wrapper.h"

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

typedef struct {
    osThreadId    id;
    osThreadDef_t def;
} rda_thread_data_t;
typedef rda_thread_data_t* rda_thread_t;

extern unsigned char rda_mac_addr[6];
extern unsigned int os_time;
extern unsigned int echo_flag;

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
static char conn_flag = 0;
static char ap_flag = 0;
static char ap_net_flag = 0;
static char fixip_flag = 0;
static char trans_index = 0xff;
//static rda_thread_data_t thread_recv;
static rda_socket_t rda_socket[SOCKET_NUM];
unsigned int baudrate = 921600;
#if 0
rda_thread_t rda_thread_new(const char *pcName,
                            void (*thread)(void *arg),
                            void *arg, int stacksize, int priority, void *thread_t) {

    rda_thread_t t = (rda_thread_t)(thread_t);

#ifdef CMSIS_OS_RTX
    t->def.pthread = (os_pthread)thread;
    t->def.tpriority = (osPriority)priority;
    t->def.stacksize = stacksize;
    t->def.stack_pointer = (uint32_t*)malloc(stacksize);
    if (t->def.stack_pointer == NULL) {
      error("Error allocating the stack memory");
      return NULL;
    }
#endif
    t->id = osThreadCreate(&t->def, arg);
    if (t->id == NULL){
        error("sys_thread_new create error\n");
        return NULL;
    }
    return t;
}
#endif
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

    /* use the bit31 of index/argument to indicate which console executes this at command */
    unsigned char idx = index & 0x80000000;

    while(1){
        if(rda_socket[index].type == TCP){
            size = ((TCPSocket*)(rda_socket[index].socket))->recv((void *)recv_buf, RECV_LIMIT);
            AT_PRINT(idx, "do_recv_thread size %d\r\n", size);
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
                    AT_RESP(idx, "+LINKDOWN=%d\r\n", index);
                    return;
                }
            }
        }else if(rda_socket[index].type == UDP){
            size = ((UDPSocket*)(rda_socket[index].socket))->recvfrom(&rda_socket[index].address, (void *)recv_buf, RECV_LIMIT);
        }
        recv_buf[size] = 0;
        if(trans_index == 0xff){
            AT_RESP(idx, "+IPD=%d,%d,%s,%d,", index, size, rda_socket[index].SERVER_ADDR, rda_socket[index].SERVER_PORT);
            AT_RESP(idx, "%s\r\n",recv_buf);
        }else if(trans_index == index){
            AT_RESP(idx, "%s\r\n",recv_buf);
        }else {
            continue;
        }
    }
}

int do_ndns(cmd_tbl_t *cmd, int argc, char *argv[], unsigned char idx)
{
    ip_addr_t addr;
    int ret;
    if(argc<2){
        AT_RESP_ERROR(idx, ERROR_ARG);
        return 0;
    }

    if(conn_flag == 0){
        AT_RESP_ERROR(idx, ERROR_ABORT);
        return 0;
    }
    ret = netconn_gethostbyname(argv[1], &addr);
    if(ret != 0){
        AT_RESP_ERROR(idx, ERROR_FAILE);
        return 0;
    }

    AT_RESP_OK_EQU(idx, "%s\r\n", inet_ntoa(addr));

    return 0;
}


int do_nmode(cmd_tbl_t *cmd, int argc, char *argv[], unsigned char idx)
{
    int last, size, index;
    unsigned int cnt = 0, time = os_time, tmp_len;
    unsigned char send_buf[SEND_LIMIT+1];
    osStatus status;

    if(argc<2){
        AT_PRINT(idx, "Please set socket index!\r\n");
        AT_RESP_ERROR(idx, ERROR_ARG);
        return 0;
    }

    index = atoi(argv[1]);

    if(rda_socket[index].used != 1){
        AT_PRINT(idx, "Socket not in used!\r\n");
        AT_RESP_ERROR(idx, ERROR_ABORT);
        return 0;
    }

    AT_PRINT(idx, "enter transparent transmission mode!\r\n");
    //rda_thread_new("tcp_send", do_recv_thread, (void *)&index, 1024*4, osPriorityNormal, &thread_recv);
    trans_index = index;
    while(1) {
        tmp_len = console_fifo_get(&send_buf[cnt], SEND_LIMIT-cnt, idx);
        if(cnt==0 && tmp_len != 0){
            cnt += tmp_len;
            if(send_buf[0] == '+'){
                time = os_time;
                do{
                    tmp_len = console_fifo_get(&send_buf[cnt], SEND_LIMIT-cnt, idx);
                    cnt += tmp_len;
                }while(cnt<3 && os_time-time < 200);

                if(cnt==3 && send_buf[1]=='+' && send_buf[2]=='+'){
                    osDelay(20);
                    if(!console_fifo_len(idx)){
                        console_putc('a', idx);
                        time = os_time;
                        do{
                            tmp_len = console_fifo_get(&send_buf[cnt], SEND_LIMIT-cnt, idx);
                            cnt += tmp_len;
                        }while(tmp_len == 0 && os_time-time<3000);
                        if(cnt==4 && send_buf[3]=='a'){
                            osDelay(20);
                            if(!console_fifo_len(idx)){
 #if 1
                                if(rda_socket[index].type == TCP){
                                    size = ((TCPSocket*)(rda_socket[index].socket))->close();
                                    delete (TCPSocket*)(rda_socket[index].socket);
                                }else if(rda_socket[index].type == UDP){
                                    size = ((UDPSocket*)(rda_socket[index].socket))->close();
                                    delete (UDPSocket*)(rda_socket[index].socket);
                                }
                                status = osThreadTerminate((osThreadId)(rda_socket[index].threadid));
                                AT_PRINT(idx, "exit send transparent transmition! status = %d\r\n", status);
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
                AT_PRINT(idx, "tcp send size %d\r\n", size);
            }else if(rda_socket[index].type == UDP){
                size = ((UDPSocket*)(rda_socket[index].socket))->sendto(rda_socket[index].SERVER_ADDR, \
                    rda_socket[index].SERVER_PORT, (void *)send_buf, cnt);
                cnt = 0;
                AT_PRINT(idx, "udp send size %d\r\n", size);
            }
        }
        last = console_fifo_len(idx);
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
int do_nsend(cmd_tbl_t *cmd, int argc, char *argv[], unsigned char idx)
{
    unsigned int index, total_len, len, tmp_len, size;
    unsigned char *buf;

    if(argc<3){
        AT_PRINT(idx, "Please set socket index!\r\n");
        AT_RESP_ERROR(idx, ERROR_ARG);
        return 0;
    }

    index = atoi(argv[1]);
    total_len = atoi(argv[2]);

    if(rda_socket[index].used != 1){
        AT_PRINT(idx, "Socket not in used!\r\n");
        AT_RESP_ERROR(idx, ERROR_ABORT);
    }

    if(total_len > SEND_LIMIT){
        AT_PRINT(idx, "Send data len longger than %d!\r\n", SEND_LIMIT);
        AT_RESP_ERROR(idx, ERROR_ARG);
    }

    buf = (unsigned char *)malloc(total_len);
    len = 0;

    while(len < total_len) {
        tmp_len = console_fifo_get(&buf[len], total_len-len, idx);
        len += tmp_len;
    }

    if(rda_socket[index].type == TCP){
        size = ((TCPSocket*)(rda_socket[index].socket))->send((void *)buf, total_len);
        AT_PRINT(idx, "tcp send size %d\r\n", size);
    }else if(rda_socket[index].type == UDP){
        size = ((UDPSocket*)(rda_socket[index].socket))->sendto(rda_socket[index].SERVER_ADDR, \
            rda_socket[index].SERVER_PORT, (void *)buf, total_len);
        AT_PRINT(idx, "udp send size %d\r\n", size);
    }

    free(buf);
    if(size == total_len)
        AT_RESP_OK(idx);
    else
        AT_RESP_ERROR(idx, ERROR_FAILE);

    return 0;
}

int do_nstop(cmd_tbl_t *cmd, int argc, char *argv[], unsigned char idx)
{
    int index;
    if(argc<2){
        AT_PRINT(idx, "Please set socket index!\r\n");
        AT_RESP_ERROR(idx, ERROR_ARG);
        return 0;
    }

    index = atoi(argv[1]);
    if(rda_socket[index].used != 1){
        AT_PRINT(idx, "Socket not in used!\r\n");
        AT_RESP_ERROR(idx, ERROR_ABORT);
    }
    if(rda_socket[index].type == TCP){
        ((TCPSocket*)(rda_socket[index].socket))->close();
    }else if(rda_socket[index].type == UDP){
        ((UDPSocket*)(rda_socket[index].socket))->close();
    }
    rda_socket[index].used = 0;
    delete rda_socket[index].socket;
    AT_RESP_OK(idx);
    return 0;
}

int do_nstart(cmd_tbl_t *cmd, int argc, char *argv[], unsigned char idx)
{
    int res, index;

    if(argc<3){
        AT_PRINT(idx, "Please set server ip and server port!\r\n");
        AT_RESP_ERROR(idx, ERROR_ARG);
        return 0;
    }
    for(index=0; index<SOCKET_NUM; index++)
        if(rda_socket[index].used == 0)
            break;

    if(index == SOCKET_NUM){
        AT_RESP_ERROR(idx, ERROR_FAILE);
        return 0;
    }

    /* add @2017.08.15 use the bit31 to indicate which console execute this command: 1 means UART1_IDX  */
    index = index | (idx << 31);

    memcpy(rda_socket[index].SERVER_ADDR, argv[2], strlen(argv[2]));
    rda_socket[index].SERVER_PORT = atoi(argv[3]);
    AT_PRINT(idx, "ip %s port %d\r\n", rda_socket[index].SERVER_ADDR, rda_socket[index].SERVER_PORT);
    rda_socket[index].address = SocketAddress(rda_socket[index].SERVER_ADDR, rda_socket[index].SERVER_PORT);

    if(!strcmp(argv[1], "TCP")){
        TCPSocket* tcpsocket = new TCPSocket(&wifi);
        rda_socket[index].type = TCP;
        rda_socket[index].socket = (void *)tcpsocket;
        res = tcpsocket->connect(rda_socket[index].address);
        if(res != 0){
            AT_PRINT(idx, "connect failed res = %d\r\n", res);
            AT_RESP_ERROR(idx, ERROR_FAILE);
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
        AT_RESP_ERROR(idx, ERROR_ARG);
        memset(&rda_socket[index], 0, sizeof(rda_socket_t));
        return 0;
    }
    rda_socket[index].used = 1;

    rda_socket[index].threadid = rda_thread_new(NULL, do_recv_thread, (void *)&index, 1024*4, osPriorityNormal);
    AT_RESP_OK_EQU(idx, "%d\r\n", index);
    return 0;
}

int do_nlink(cmd_tbl_t *cmd, int argc, char *argv[], unsigned char idx)
{
    int i, n=0;

    for(i=0; i<SOCKET_NUM; i++)
        if(rda_socket[i].used == 1)
            n++;

    AT_RESP_OK_EQU(idx, "%d\r\n", n);

    for(i=0; i<SOCKET_NUM; i++){
        if(rda_socket[i].used == 1)
            AT_RESP_OK_EQU(idx, "%d,%s,%s,%d,%d\r\n", i, rda_socket[i].type ? "UDP":"TCP", rda_socket[i].SERVER_ADDR, \
                rda_socket[i].SERVER_PORT, rda_socket[i].LOCAL_PORT);
    }

    return 0;
}

extern unsigned char ping_console_idx;
int do_nping(cmd_tbl_t *cmd, int argc, char *argv[], unsigned char idx)
{
    int count, interval, len, echo_level;

    if (argc < 6) {
        AT_RESP_ERROR(idx, ERROR_ARG);
        return 0;
    }

    count = atoi(argv[2]);
    interval = atoi(argv[3]);
    len = atoi(argv[4]);
    echo_level = atoi(argv[5]);
    if (count < 0 || interval < 0 || len < 0) {
        AT_RESP_ERROR(idx, ERROR_ARG);
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

    ping_console_idx = idx;
    ping_init((const char*)argv[1], count, interval, len, echo_level);
    return 0;
}

int do_wsfixip(cmd_tbl_t *cmd, int argc, char *argv[], unsigned char idx)
{
    int enable, flag;
    unsigned int ip, msk, gw;
    if (argc < 3) {
        AT_RESP_ERROR(idx, ERROR_ARG);
        return 0;
    }

    if(conn_flag == 1){
        AT_RESP_ERROR(idx, ERROR_ABORT);
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
    AT_RESP_OK(idx);
    return 0;
}

int do_wsscan(cmd_tbl_t *cmd, int argc, char *argv[], unsigned char idx)
{
    int cnt=0, num=0, i;
    rda5981_scan_result *bss_list = NULL;

    cnt = wifi.scan(NULL, 0);

    if(*argv[1] == '?'){
        bss_list = (rda5981_scan_result *)malloc(cnt * sizeof(rda5981_scan_result));
        memset(bss_list, 0, cnt * sizeof(rda5981_scan_result));
        num = wifi.scan_result(bss_list, cnt);
        AT_RESP_OK_EQU(idx, "%d\r\n", num);
        for(i=0; i<num; i++){
            AT_RESP_OK_EQU(idx, "%02x:%02x:%02x:%02x:%02x:%02x,", bss_list[i].BSSID[0], bss_list[i].BSSID[1], bss_list[i].BSSID[2], bss_list[i].BSSID[3], bss_list[i].BSSID[4], bss_list[i].BSSID[5]);
            AT_RESP(idx, "%d,%d,%s\r\n", bss_list[i].channel, bss_list[i].RSSI, bss_list[i].SSID);
        }
        free(bss_list);
    }else{
        AT_RESP_OK(idx);
    }

    return 0;
}

int do_wapstop( cmd_tbl_t *cmd, int argc, char *argv[], unsigned char idx)
{
    if(ap_flag == 0){
        AT_RESP_ERROR(idx, ERROR_ABORT);
        return 0;
    }
    wifi.stop_ap(0);
    ap_flag = 0;

    AT_RESP_OK(idx);
    return 0;
}

int do_wap( cmd_tbl_t *cmd, int argc, char *argv[], unsigned char idx)
{
    unsigned char channel = 0;
    unsigned int ip, msk, gw, dhcps, dhcpe, flag;
    char ips[IP_LENGTH], msks[IP_LENGTH], gws[IP_LENGTH], dhcpss[IP_LENGTH], dhcpes[IP_LENGTH];
    if (argc < 1) {
        AT_RESP_ERROR(idx, ERROR_ARG);
        return 0;
    }

    if(*argv[1] == '?'){
        if(ap_flag == 1){
            AT_RESP_OK_EQU(idx, "ssid:%s, pw:%s, ip:%s, BSSID:%s\r\n", ssid_ap, pw_ap, wifi.get_ip_address_ap(), wifi.get_mac_address_ap());
        }else{
            AT_RESP_OK_EQU(idx, "ssid:, pw:, ip:0.0.0.0, BSSID:00:00:00:00:00:00\r\n");
        }
        return 0;
    }

    if(ap_flag == 1){
        AT_RESP_ERROR(idx, ERROR_ABORT);
        return 0;
    }

    memset(ssid_ap, 0, sizeof(ssid_ap));
    memset(pw_ap, 0, sizeof(pw_ap));

    if(argc == 1){
        if(rda5981_flash_read_ap_data(ssid_ap, pw_ap, &channel)){
            AT_RESP_ERROR(idx, ERROR_FAILE);
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
    AT_RESP_OK(idx);

    return 0;
}

int conn(char *ssid, char *pw)
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
    ret = wifi.connect(ssid, pw, NULL, NSAPI_SECURITY_NONE, 0);
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

int do_wsc( cmd_tbl_t *cmd, int argc, char *argv[], unsigned char idx)
{
    int ret, flag;
    const char *ip_addr, *mac_addr;

    if (argc < 2) {
        AT_RESP_ERROR(idx, ERROR_ARG);
        return 0;
    }

    if(conn_flag == 1){
        AT_RESP_ERROR(idx, ERROR_ABORT);
        AT_PRINT(idx, "error! Has been connected!");
        return 0;
    }

    wifi.scan(NULL, 0);//necessary
    ret = rda5981_getssid_from_smartconfig(ssid, pw);
    if (ret) {
        AT_PRINT(idx, "smartconfig fail, ret: %d \r\n", ret);
        AT_RESP_ERROR(idx, ERROR_FAILE);
        return 0;
    } else {
        AT_PRINT(idx, "smartconfig success, ssid = \"%s\", pw = \"%s\" \r\n", ssid, pw);
    }

    flag = atoi(argv[1]);
    ret = conn(ssid, pw);
    if(ret == 0){
        if(flag == 1)
            rda5981_flash_write_sta_data(ssid, pw);
        ip_addr = wifi.get_ip_address();
        mac_addr = wifi.get_mac_address();
        rda5981_sendrsp_to_host(ip_addr, mac_addr, (void *)&wifi);
        AT_RESP_OK(idx);
    }else{
        AT_RESP_ERROR(idx, ERROR_FAILE);
    }
    return 0;
}

int do_wsconn( cmd_tbl_t *cmd, int argc, char *argv[], unsigned char idx)
{
    int ret, flag;

    if (argc < 1) {
        AT_RESP_ERROR(idx, ERROR_ARG);
        return 0;
    }

    if(*argv[1] == '?'){
        if(conn_flag == 1){
            wifi.update_rssi();
                AT_RESP_OK_EQU(idx, "ssid:%s, RSSI:%ddb, ip:%s, BSSID: "MAC_FORMAT"\r\n", wifi.get_ssid(), wifi.get_rssi(), wifi.get_ip_address(), MAC2STR(wifi.get_BSSID()));
        }else{
                AT_RESP_OK_EQU(idx, "ssid:, RSSI:0db, ip:0.0.0.0\r\n");
        }
        return 0;
    }

    if(conn_flag == 1){
        AT_RESP_ERROR(idx, ERROR_ABORT);
        AT_PRINT(idx, "error! Has been connected!");
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
            AT_PRINT(idx, "get ssid from flash: ssid:%s, pass:%s\r\n", ssid, pw);
        }else{
            AT_RESP_ERROR(idx, ERROR_ARG);
            return 0;
        }
    }

    AT_PRINT(idx, "ssid %s pw %s\r\n", ssid, pw);

    ret = conn(ssid, pw);
    if(ret == 0){
        if(flag == 1)
            rda5981_flash_write_sta_data(ssid, pw);
        AT_RESP_OK(idx);
    }else{
        AT_RESP_ERROR(idx, ERROR_FAILE);
    }
    return 0;
}

int do_disconn( cmd_tbl_t *cmd, int argc, char *argv[], unsigned char idx)
{
    if(conn_flag == 0){
        AT_RESP_ERROR(idx, ERROR_ABORT);
        return 0;
    }

    wifi.disconnect();
    while(wifi.get_ip_address() != NULL)
        Thread::wait(20);
    conn_flag = 0;
    AT_RESP_OK(idx);
    return 0;
}

int do_dbg( cmd_tbl_t *cmd, int argc, char *argv[], unsigned char idx)
{
    int arg;

    if (argc < 3) {
        AT_RESP_ERROR(idx, ERROR_ARG);
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
        AT_RESP_ERROR(idx, ERROR_ARG);
        return 0;
    }

    AT_RESP_OK(idx);

    return 0;
}

int do_uart( cmd_tbl_t *cmd, int argc, char *argv[], unsigned char idx)
{
    if (argc < 2) {
        AT_RESP_ERROR(idx, ERROR_ARG);
        return 0;
    }

    //inquire
    if(*argv[1] == '?'){
        AT_RESP_OK_EQU(idx, "%d\r\n", baudrate);
        return 0;
    }

    baudrate = atoi(argv[2]);
    console_set_baudrate(baudrate);
    if(atoi(argv[1]) == 1)
        rda5981_flash_write_uart(&baudrate);
    AT_RESP_OK(idx);

    return 0;
}

int do_wsmac( cmd_tbl_t *cmd, int argc, char *argv[], unsigned char idx)
{
    char *mdata, mac[6], i;

    if(*argv[1] == '?'){
        rda_get_macaddr((unsigned char *)mac, 0);
        AT_RESP_OK_EQU(idx, "%02x:%02x:%02x:%02x:%02x:%02x\r\n",
            mac[0], mac[1], mac[2], mac[3], mac[4], mac[5]);
        return 0;
    }

    if(conn_flag == 1 || ap_flag == 1){
        AT_RESP_ERROR(idx, ERROR_ABORT);
        return 0;
    }

    if(strlen(argv[1]) != 12){
        AT_RESP_ERROR(idx, ERROR_ARG);
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
        AT_RESP_ERROR(idx, ERROR_ARG);
        return 0;
    }
    memcpy(rda_mac_addr, mac, 6);
    rda5981_flash_write_mac_addr(rda_mac_addr);
    AT_RESP_OK(idx);

    return 0;
}

int do_wamac( cmd_tbl_t *cmd, int argc, char *argv[], unsigned char idx)
{
    char mac[6];

    if(*argv[1] == '?'){
        rda_get_macaddr((unsigned char *)mac, 1);
        AT_RESP_OK_EQU(idx, "%02x:%02x:%02x:%02x:%02x:%02x\r\n",
            mac[0], mac[1], mac[2], mac[3], mac[4], mac[5]);
        return 0;
    }
    AT_RESP_ERROR(idx, ERROR_ARG);
/*
    char *mdata, i;
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

int do_wasta( cmd_tbl_t *cmd, int argc, char *argv[], unsigned char idx)
{
    int cnt = 5, num, i;
    rda5981_apsta_info * sta_list;

    sta_list = (rda5981_apsta_info *)malloc(5 * sizeof(rda5981_apsta_info));
    memset(sta_list, 0, cnt * sizeof(rda5981_apsta_info));
    num = wifi.ap_join_info(sta_list, cnt);
    AT_RESP_OK_EQU(idx, "%d\r\n", num);
    for(i=0; i<num; i++){
        AT_RESP_OK_EQU(idx, "%02x:%02x:%02x:%02x:%02x:%02x,", sta_list[i].mac[0], sta_list[i].mac[1], sta_list[i].mac[2], sta_list[i].mac[3], sta_list[i].mac[4], sta_list[i].mac[5]);
        AT_RESP(idx, "%s\r\n", inet_ntoa(sta_list[i].ip));
    }
    free(sta_list);
    return 0;
}

int do_wanet(cmd_tbl_t *cmd, int argc, char *argv[], unsigned char idx)
{
    unsigned int ip, msk, gw, dhcps, dhcpe, flag;

    if (argc < 7) {
        AT_RESP_ERROR(idx, ERROR_ARG);
        return 0;
    }

    if(ap_flag == 1){
        AT_RESP_ERROR(idx, ERROR_ABORT);
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

    AT_RESP_OK(idx);
    return 0;
}

int do_ver( cmd_tbl_t *cmd, int argc, char *argv[], unsigned char idx)
{
    AT_RESP_OK_EQU(idx, "%s\r\n", version);
    return 0;
}

int do_userdata( cmd_tbl_t *cmd, int argc, char *argv[], unsigned char idx)
{
    int len, ret, tmp_len = 0;
    unsigned char *buf;

    if (argc < 2) {
        AT_RESP_ERROR(idx, ERROR_ARG);
        return 0;
    }

    if(*argv[1] == '?'){
        len  = rda5981_flash_read_3rdparter_data_length();
        if (len < 0) {
            AT_PRINT(idx, "No user data in flah\r\n");
            AT_RESP_ERROR(idx, ERROR_FAILE);
            return 0;
        }

        buf = (unsigned char *)malloc(len+1);
        memset(buf, 0, len+1);

        ret = rda5981_flash_read_3rdparter_data(buf, len);
        if(ret < 0)
            AT_PRINT(idx, "read flash error, error %d\r\n", ret);
        else
            AT_PRINT(idx, "Data read complete, len:%d\r\n", ret);

        buf[len] = 0;
        AT_RESP_OK_EQU(idx, "%d,%s\r\n", len, buf);
        free(buf);
        return 0;
    }

    len = atoi(argv[1]);
    buf = (unsigned char *)malloc(len);
    memset(buf, 0, len);

    do{
        unsigned int size;
        size = console_fifo_get(buf+tmp_len, len-tmp_len, idx);
        tmp_len += size;
    }while(tmp_len < len);

    ret = rda5981_flash_write_3rdparter_data(buf, len);
    if(ret < 0){
        AT_PRINT(idx, "write flash error, error %d\r\n", ret);
        AT_RESP_ERROR(idx, ERROR_FAILE);
    }else{
        AT_PRINT(idx, "Data write complete\r\n");
        AT_RESP_OK(idx);
    }

    free(buf);
    return 0;
}

int do_at( cmd_tbl_t *cmd, int argc, char *argv[], unsigned char idx)
{
    AT_RESP_OK(idx);
    return 0;
}

int do_rst( cmd_tbl_t *cmd, int argc, char *argv[], unsigned char idx)
{
    AT_RESP_OK(idx);
    rda_wdt_softreset();
    return 0;

}

int do_echo( cmd_tbl_t *cmd, int argc, char *argv[], unsigned char idx)
{
    int flag;

    if (argc < 2) {
        AT_RESP_ERROR(idx, ERROR_ARG);
        return 0;
    }
    flag = atoi(argv[1]);

    if(flag != 0 && flag != 1){
        AT_RESP_ERROR(idx, ERROR_ARG);
        return 0;
    }

    echo_flag = flag;
    AT_RESP_OK(idx);
    return 0;

}

int do_restore( cmd_tbl_t *cmd, int argc, char *argv[], unsigned char idx)
{
    if(argc < 1) {
        AT_RESP_ERROR(idx, ERROR_ARG);
        return 0;
    }

    rda5981_flash_erase_ap_data();
    rda5981_flash_erase_ap_net_data();
    rda5981_flash_erase_dhcp_data();
    rda5981_flash_erase_sta_data();
    rda5981_flash_erase_uart();

    AT_RESP_OK(idx);
    return 0;
}


int do_h( cmd_tbl_t *cmd, int argc, char *argv[], unsigned char idx)
{
    cmd_tbl_t *cmdtemp = NULL;
    int i;

    for (i = 0; i<(int)cmd_cntr; i++) {
        cmdtemp = &cmd_list[i];
        if(cmdtemp->usage) {
            AT_RESP_OK_EQU(idx, "%s\r\n", cmdtemp->usage);
        }
    }
    return 0;
}

int do_sleep( cmd_tbl_t *cmd, int argc, char *argv[], unsigned char idx)
{
    int enable;
    if(argc < 2) {
        AT_RESP_ERROR(idx, ERROR_ARG);
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
        AT_RESP_ERROR(idx, ERROR_ARG);
    }
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
            "AT+USERDATA",      2,   do_userdata,
            "AT+USERDATA        - write/read user data"
        },
        {
            "AT+SLEEP",         2,   do_sleep,
            "AT+SLEEP           - enable/disable sleep"
        },
        {
            "AT+RESTORE",       1,   do_restore,
            "AT+RESTORE         - restore default config"
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
            "AT+WDBG",          3,   do_dbg,
            "AT+WDBG            - adjust debug level"
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
    console_uart1_init();
    console_uart0_init();
    rda5991h_smartlink_irq_init();
    add_cmd();
}

