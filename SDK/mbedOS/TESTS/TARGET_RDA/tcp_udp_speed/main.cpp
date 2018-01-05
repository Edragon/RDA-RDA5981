#include "mbed.h"
#include "rtos.h"
#include "WiFiStackInterface.h"
#include "NetworkStack.h"
#include "TCPSocket.h"
#include "console.h"
#include "at.h"
#include "cmsis_os.h"

typedef struct {
    osThreadId    id;
    osThreadDef_t def;
} rda_thread_data_t;
typedef rda_thread_data_t* rda_thread_t;

static rda_thread_data_t thread_recv;

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
    if (t->id == NULL) {
        error("sys_thread_new create error\n");
        return NULL;
    }
    return t;
}

typedef struct {
    void *socket;
    int type;
    char SERVER_ADDR[20];
    int SERVER_PORT;
    int LOCAL_PORT; //used for UDP
} rda_socket_t;

typedef enum {
    TCP = 1,
    UDP,
} socket_type_t;

extern unsigned int os_time;
#define BUF_LENGTH (10 * 1460)
char tx_buffer[BUF_LENGTH] = {0};
char rx_buffer[BUF_LENGTH] = {0};
const int TCP_LOOPS = 4000;
const int UDP_LOOPS = 5000;

WiFiStackInterface wifi;
static char ssid[32+1];
static char pw[64+1];
static char conn_flag = 0;

rda_socket_t rda_socket;
UDPSocket udpsocket;
TCPSocket *tcpsocket;

int do_wsconn( cmd_tbl_t *cmd, int argc, char *argv[])
{
    char *ssid_t, *pw_t;
    const char *ip_addr;
    int ret;

    if (argc < 1) {
        RESP_ERROR(ERROR_ARG);
        return 0;
    }

    if (*argv[1] == '?') {
        if (conn_flag == 1) {
            wifi.update_rssi();
            RESP_OK_EQU("ssid:%s, RSSI:%ddb, ip:%s\r\n", wifi.get_ssid(), wifi.get_rssi(), wifi.get_ip_address());
        } else {
            RESP_OK_EQU("ssid:, RSSI:0db, ip:0.0.0.0\r\n");
        }
        return 0;
    }

    if (conn_flag == 1) {
        RESP_ERROR(ERROR_ABORT);
        RDA_AT_PRINT("error! Has been connected!");
        return 0;
    }

    memset(ssid, 0, sizeof(ssid));
    memset(pw, 0, sizeof(pw));

    if (argc > 1)
        memcpy(ssid, argv[1], strlen(argv[1]));

    if (argc > 2)
        memcpy(pw, argv[2], strlen(argv[2]));

    if (strlen(ssid) != 0)
        ssid_t = ssid;
    else
        ssid_t = NULL;

    if (strlen(pw) != 0)
        pw_t = pw;
    else
        pw_t = NULL;

    RDA_AT_PRINT("ssid %s pw %s\r\n", ssid_t, pw_t);

    ret = wifi.connect(ssid_t, pw_t, NULL, NSAPI_SECURITY_NONE, 0);
    if (ret != 0) {
        RESP_ERROR(ERROR_FAILE);
        return 0;
    }
    ip_addr = wifi.get_ip_address();
    if (ip_addr) {
        printf("Connect %s success! Client IP Address is %s\r\n", ssid_t, ip_addr);
        conn_flag = 1;
        RESP_OK();
    } else {
        RDA_AT_PRINT("No Client IP Address\r\n");
        RESP_ERROR(ERROR_FAILE);
    }

    return 0;
}

void tcp_send_test (const char *address, uint16_t port)
{
    int ret;
    long t1, rLen, totalLen = 0;

    SocketAddress addr(address, port);

    if (!addr) {
        printf("address error\r\n");
        return;
    }
    memset(tx_buffer, 0xff, BUF_LENGTH);

    tcpsocket = new TCPSocket(&wifi);

    ret = (int)tcpsocket->connect(addr);
    if (ret != 0) {
        printf("connect err = %d\r\n", ret);
        return;
    }
    printf("tcp send test begin \r\n");
    t1 = os_time;
    for (int i = 0; i < TCP_LOOPS; ++i) {
        rLen = tcpsocket->send(tx_buffer, BUF_LENGTH);

        if (rLen <= 0) {
            printf("ERROR: TCP send error, len is %d !\r\n", rLen);
            break;
        }
        totalLen += rLen;
        if (os_time - t1 > 1000) {
            printf("tcp send Speed %0.5f KB/s\r\n", totalLen * 1000.0 / 1024 / (os_time - t1));
            t1 = os_time;
            totalLen = 0;
        }
    }
    printf("tcp send test end \r\n");
    tcpsocket->close();
    delete tcpsocket;
}

void tcp_recv_test (const char *address, uint16_t port)
{
    int ret;
    long t1, rLen, totalLen = 0;

    SocketAddress addr(address, port);
    if (!addr) {
        printf("address error\r\n");
        return;
    }
    memset(rx_buffer, 0, BUF_LENGTH);

    tcpsocket = new TCPSocket(&wifi);

    ret = (int)tcpsocket->connect(addr);
    if (ret != 0) {
        printf("connect err = %d\r\n", ret);
        return;
    }

    printf("tcp recv test begin \r\n");
    t1 = os_time;
    do {
        rLen = tcpsocket->recv((void *)rx_buffer, BUF_LENGTH);
        if (rLen <= 0) {
            printf("ERROR: TCP recv erropr, len is %d £¡\r\n", rLen);
            break;
        }
        totalLen += rLen;
        if (os_time - t1 > 1000) {
            printf("tcp recv Speed %0.5f KB/s\r\n", totalLen * 1000.0 / 1024 / (os_time - t1));
            t1 = os_time;
            totalLen = 0;
        }
    } while (rLen > 0);

    printf("tcp recv test end \r\n");
    tcpsocket->close();
    delete tcpsocket;
}

void udp_send_test (const char *address, uint16_t port, uint16_t local_port) {
    long t1, rLen, totalLen = 0;

    SocketAddress addr(address, port);
    printf("port is %d \r\n", port);
    if (!addr) {
        printf("address error\r\n");
        return;
    }
    memset(tx_buffer, 0xff, BUF_LENGTH);

    udpsocket.open(&wifi);
    udpsocket.bind(local_port);

    printf("udp send test begin \r\n");
    t1 = os_time;
    for (int i = 0; i < UDP_LOOPS; ++i) {
        rLen = udpsocket.sendto(address, port, tx_buffer, 3 * 1460);
        wait(0.01);

        if (rLen <= 0) {
            printf("ERROR:UDP send error, len is %d £¡\r\n", rLen);
            break;
        }
        totalLen += rLen;
        if (os_time - t1 > 1000) {
            printf("udp send Speed %0.5f KB/s\r\n", totalLen * 1000.0 / 1024 / (os_time - t1));
            t1 = os_time;
            totalLen = 0;
        }
    }
    printf("udp send test end \r\n");
    udpsocket.close();
}

void udp_recv_test (const char *address, uint16_t port, uint16_t local_port) {
    long t1, rLen, totalLen= 0;

    SocketAddress addr(address, port);
    if (!addr) {
        printf("address error\r\n");
        return;
    }
    memset(rx_buffer, 0, BUF_LENGTH);

    udpsocket.open(&wifi);
    udpsocket.bind(local_port);

    printf("udp recv test begin \r\n");
    t1 = os_time;
    do {
        rLen = udpsocket.recvfrom(&addr,(void *) rx_buffer, BUF_LENGTH);
        //printf("recv %d Bytes \n", rLen);
        if (rLen <= 0) {
            printf("ERROR: UDP recv error, len is %d £¡\r\n", rLen);
            break;
        }
        totalLen += rLen;
        if (os_time - t1 > 1000) {
            printf("udp recv Speed %0.5f KB/s\r\n", totalLen * 1000.0 / 1024 / (os_time - t1));
            t1 = os_time;
            totalLen = 0;
        }
    } while (rLen > 0);
    printf("udp recv test end \r\n");
}

void do_recv_thread(void *argument) {
    unsigned int type;
    type = *(int *)argument;

    if (type == TCP) {
        tcp_recv_test(rda_socket.SERVER_ADDR, rda_socket.SERVER_PORT);
    } else {
        udp_recv_test(rda_socket.SERVER_ADDR,  rda_socket.SERVER_PORT, rda_socket.LOCAL_PORT);
    }
}

int do_nstart(cmd_tbl_t *cmd, int argc, char *argv[]) {
    int index, status, sta;

    if (argc < 4) {
        printf("Please set server ip and server port!\r\n");
        RESP_ERROR(ERROR_ARG);
        return 0;
    }
    if (conn_flag == 0) {
        printf("Please connect ap!\r\n");
        RESP_ERROR(ERROR_ABORT);
        return 0;
    }
    status = osThreadGetState(thread_recv.id);
    RDA_AT_PRINT("thread status is %d \r\n", status);
    if (status != 0) {
        if (rda_socket.type == TCP) {
            printf("close tcp recv \r\n");
            if (tcpsocket) {
                tcpsocket->close();
                delete tcpsocket;
            }
        } else if (rda_socket.type == UDP) {
            printf("close udp recv \r\n");
            udpsocket.close();
        }
        sta = osThreadTerminate(thread_recv.id);
        RDA_AT_PRINT("sta is %d \r\n", sta);
    }

    memcpy(rda_socket.SERVER_ADDR, argv[3], strlen(argv[3]));
    rda_socket.SERVER_PORT = atoi(argv[4]);
    RDA_AT_PRINT("ip is %s , port is %d \r\n", rda_socket.SERVER_ADDR, rda_socket.SERVER_PORT );


    if (!strcmp(argv[1], "TCP")) {
        rda_socket.type = TCP;
        if (!strcmp(argv[2], "TX")) {
            tcp_send_test(rda_socket.SERVER_ADDR, rda_socket.SERVER_PORT);
        } else if (!strcmp(argv[2], "RX")) {
            rda_thread_new("tcp_recv", do_recv_thread, (void *)&rda_socket.type, 512*4, osPriorityNormal, &thread_recv);
        }
    } else if (!strcmp(argv[1], "UDP")) {
        rda_socket.type = UDP;
        if (argc < 5) {
            printf("Please set udp local port!\r\n");
            RESP_ERROR(ERROR_ARG);
            return 0;
        }

        rda_socket.LOCAL_PORT = atoi(argv[5]);
        RDA_AT_PRINT("local port is %d\r\n", rda_socket.LOCAL_PORT);

        if (!strcmp(argv[2], "TX")) {
            udp_send_test(rda_socket.SERVER_ADDR, rda_socket.SERVER_PORT, rda_socket.LOCAL_PORT);
        } else if (!strcmp(argv[2], "RX")) {
            rda_thread_new("tcp_recv", do_recv_thread, (void *)&rda_socket.type, 512*4, osPriorityNormal, &thread_recv);
        }
    } else {
        RESP_ERROR(ERROR_ARG);
    }
    RESP_OK_EQU("%d\r\n", index);
    return 0;
}

int do_disconn( cmd_tbl_t *cmd, int argc, char *argv[]) {
    int status, sta;

    if (conn_flag == 0) {
        RESP_ERROR(ERROR_ABORT);
        return 0;
    }

    status = osThreadGetState(thread_recv.id);
    RDA_AT_PRINT("thread_recv  status is %d \r\n", status);
    if (status != 0) {
        if (rda_socket.type == TCP) {
            RDA_AT_PRINT("close tcp recv \r\n");
            if (tcpsocket) {
                tcpsocket->close();
                delete tcpsocket;
            }
        } else if (rda_socket.type == UDP) {
            RDA_AT_PRINT("close udp recv \r\n");
            udpsocket.close();
        }
        sta = osThreadTerminate(thread_recv.id);
        RDA_AT_PRINT("sta is %d \r\n",sta);
    }

    wifi.disconnect();
    while (wifi.get_ip_address() != NULL)
        Thread::wait(20);
    conn_flag = 0;
    RESP_OK();
    return 0;
}

void add_cmd() {
    int i, j;
    cmd_tbl_t cmd_list[] = {
        /*Basic CMD*/
        {
            "AT+WSCONN",        3,   do_wsconn,
            "AT+WSCONN          - start wifi connect"
        },
        {
            "AT+WSDISCONN",     1,   do_disconn,
            "AT+WSDISCONN       - disconnect"
        },
        /*NET CMD*/
        {
            "AT+NSTART",        6,   do_nstart,
            "AT+NSTART          - start tcp/udp speed test"
        },

    };
    j = sizeof(cmd_list)/sizeof(cmd_tbl_t);
    for (i = 0; i < j; i++) {
        if (0 != console_cmd_add(&cmd_list[i])) {
            RDA_AT_PRINT("Add cmd failed\r\n");
        }
    }
}

void start_at(void) {
    console_init();
    add_cmd();
}

int main() {
    printf("TCP/UDP speed test begin\r\n");
    start_at();
}

