#if defined(TARGET_UNO_91H)
#include "mbed.h"
#include "rtos.h"
#include "console.h"
#include "WiFiStackInterface.h"
#include "rda_wdt_api.h"
#include "rda_ccfg_api.h"
#include "cmsis_os.h"
#include "rda_wdt_api.h"
#include "Rda58xx.h"

static char* version = "**********RDA Software Version sta_V2.0_124**********";
static int op_mode = 0;//initial ~ 0 factory test ~ 1 function ~ 2
static char conn_flag = 0;
static WiFiStackInterface wifi;
extern unsigned char rda_mac_addr[6];
extern rda58xx _rda58xx;

int do_conn_state(cmd_tbl_t* cmd, int argc, char* argv[]) {
    if (op_mode != 1) {
        return 0;
    }

    if (argc < 1) {
        show_cmd_usage(cmd);
        return -1;
    }

#if 1
    const char* ssid;
    ssid = wifi.get_ssid();

    if (ssid[0]) {
        printf("CONNECTED! ssid:%s RSSI:%d db ip:%s\r\n", wifi.get_ssid(), wifi.get_rssi(),
               wifi.get_ip_address());
    } else {
        printf("NOT CONNECTED!\r\n");
    }

#else

    if (wifi.bss.SSID[0]) {
        printf("CONNECTED! ssid:%s RSSI:%d db\r\n", wifi.bss.SSID, wifi.bss.SSID);
    } else {
        printf("NOT CONNECTED!\r\n");
    }

#endif
    return 0;
}

int do_get_mac(cmd_tbl_t* cmd, int argc, char* argv[]) {
    if (op_mode != 1) {
        return 0;
    }

    if (argc < 1) {
        show_cmd_usage(cmd);
        return -1;
    }

    char mac[6];
    mbed_mac_address(mac);
    printf("MAC address: %02x:%02x:%02x:%02x:%02x:%02x\r\n",
           mac[0], mac[1], mac[2], mac[3], mac[4], mac[5]);

    return 0;
}

int do_set_mac(cmd_tbl_t* cmd, int argc, char* argv[]) {
    char* mdata, mac[6], i;

    if (op_mode != 1) {
        return 0;
    }

    if (argc < 2) {
        show_cmd_usage(cmd);
        return -1;
    }

    if (strlen(argv[1]) != 12) {
        printf("Error MAC address len\r\n");
        return -2;
    }

    mdata = argv[1];

    for (i = 0; i < 12; i++) {
        if (mdata[i] >= 0x41 && mdata[i] <= 0x46) { // switch 'A' to 'a'
            mdata[i] += 0x20;
        }

        if (mdata[i] >= 0x61 && mdata[i] <= 0x66) { //switch "ab" to 0xab
            mac[i] = mdata[i] - 0x57;
        }

        if (mdata[i] >= 0x30 && mdata[i] <= 0x39) {
            mac[i] = mdata[i] - 0x30;
        }

        if (i % 2 == 1) {
            mac[i / 2] = mac[i - 1] << 4 | mac[i];
        }
    }

    if (!mac_is_valid(mac)) {
        printf("MAC is ZERO\r\n");
        return 0;
    }

    memcpy(rda_mac_addr, mac, 6);
    rda5981_flash_write_mac_addr(rda_mac_addr);
    return 0;
}

int do_get_ver(cmd_tbl_t* cmd, int argc, char* argv[]) {
    if (op_mode != 1) {
        return 0;
    }

    if (argc < 1) {
        show_cmd_usage(cmd);
        return -1;
    }

    printf("Software Version: %s\r\n", version);
    return 0;
}

int do_get_ver_5856(cmd_tbl_t* cmd, int argc, char* argv[]) {
    char ver[32] = {0};
    rda58xx_at_status ret;

    if (op_mode != 1) {
        return 0;
    }

    if (argc < 1) {
        show_cmd_usage(cmd);
        return -1;
    }

    printf("do_get_ver_5856\r\n");

    while (!_rda58xx.isPowerOn()) {
        Thread::wait(100);
    }

    ret = _rda58xx.getChipVersion(ver);

    if (VACK == ret) {
        printf("5856 Software Version: %s\r\n", ver);
        return 0;
    } else {
        printf("5856 Software Version: Fail!\r\n");
        return -1;
    }
}

int do_factory_test_5856(cmd_tbl_t* cmd, int argc, char* argv[]) {
    rda58xx_at_status ret;

    if (op_mode != 1) {
        return 0;
    }

    if (argc < 1) {
        show_cmd_usage(cmd);
        return -1;
    }

    printf("do_factory_test_5856\r\n");
    ret = _rda58xx.factoryTest(FT_ENABLE);

    if (VACK == ret) {
        printf("5856 Factory Test: Succeed\r\n");
        return 0;
    } else {
        printf("5856 Factory Test: Fail\r\n");
        return -1;
    }
}


int do_write_usedata(cmd_tbl_t* cmd, int argc, char* argv[]) {

    int len, ret, tmp_len = 0;
    unsigned char* buf;

    if (op_mode != 1) {
        return 0;
    }

    if (argc < 2) {
        show_cmd_usage(cmd);
        return -1;
    }

    len = atoi(argv[1]);
    buf = (unsigned char*)malloc(len);
    memset(buf, 0, len);

    do {
        unsigned int size;
        size = console_fifo_get(buf + tmp_len, len - tmp_len);
        tmp_len += size;
    } while (tmp_len < len);

    ret = rda5981_flash_write_3rdparter_data(buf, len);

    if (ret < 0) {
        printf("write flash error, error %d\r\n", ret);
    } else {
        printf("Data write complete\r\n");
    }

    free(buf);
    return 0;
}

int do_read_usedata(cmd_tbl_t* cmd, int argc, char* argv[]) {

    int len = 0, ret = -1;
    unsigned char* buf;

    if (op_mode != 1) {
        return 0;
    }

    if (argc != 1) {
        show_cmd_usage(cmd);
        return -1;
    }

    len  = rda5981_flash_read_3rdparter_data_length();

    if (len < 0) {
        printf("No user data in flah\r\n");
        return -1;
    }

    buf = (unsigned char*)malloc(len + 1);
    memset(buf, 0, len + 1);

    ret = rda5981_flash_read_3rdparter_data(buf, len);

    if (ret < 0) {
        printf("read flash error, error %d\r\n", ret);
    } else {
        printf("Data read complete, len:%d\r\n", ret);
    }

    buf[len] = 0;
    console_puts((char*)buf);
    printf("\n");
    free(buf);
    return 0;
}

int do_reset(cmd_tbl_t* cmd, int argc, char* argv[]) {
    if (op_mode != 1) {
        return 0;
    }

    if (argc < 1) {
        show_cmd_usage(cmd);
        return -1;
    }

    printf("SOFTWARE RESET!!!!\r\n");
    rda_wdt_softreset();
    return 0;

}

int do_conn(cmd_tbl_t* cmd, int argc, char* argv[]) {
    char* ssid_t, *pw_t;
    const char* ip_addr;
    char ssid[20];
    char pw[20];

    if (op_mode != 1) {
        return 0;
    }

    if (conn_flag == 1) {
        printf("error! Has been connected!");
        return -1;
    }

    printf("OK, start connect\r\n");

    if (argc < 1) {
        show_cmd_usage(cmd);
        return -1;
    }

    memset(ssid, 0, sizeof(ssid));
    memset(pw, 0, sizeof(pw));

    if (argc > 1) {
        memcpy(ssid, argv[1], strlen(argv[1]));
    }

    if (argc > 2) {
        memcpy(pw, argv[2], strlen(argv[2]));
    }

    if (strlen(ssid) != 0) {
        ssid_t = ssid;
    } else {
        ssid_t = NULL;
    }

    if (strlen(pw) != 0) {
        pw_t = pw;
    } else {
        pw_t = NULL;
    }

    printf("ssid %s pw %s\r\n", ssid_t, pw_t);

    wifi.connect(ssid_t, pw_t);

    while (!wifi.get_ip_address());

    ip_addr = wifi.get_ip_address();

    if (ip_addr) {
        printf("Client IP Address is %s\r\n", ip_addr);
    } else {
        printf("No Client IP Address\r\n");
    }

    conn_flag = 1;
    //rda5981_flash_write_smartconfig_data();
    return 0;
}

int do_disconn(cmd_tbl_t* cmd, int argc, char* argv[]) {
    if (op_mode != 1) {
        return 0;
    }

    if (argc < 1) {
        show_cmd_usage(cmd);
        return -1;
    }

    if (conn_flag == 0) {
        printf("Not connectted!\r\n");
        return 0;
    }

    wifi.disconnect();

    while (wifi.get_ip_address() != NULL) {
        Thread::wait(20);
    }

    conn_flag = 0;
    //printf("OK, Disconnect successful\r\n");
    return 0;
}

int do_set_baud(cmd_tbl_t* cmd, int argc, char* argv[]) {
    unsigned int baudrate;

    if (op_mode != 1) {
        return 0;
    }

    if (argc < 2) {
        show_cmd_usage(cmd);
        return -1;
    }

    printf("OK, do set baud\r\n");
    baudrate = atoi(argv[1]);
    console_set_baudrate(baudrate);

    return 0;
}

void add_cmd() {
    int i;
    cmd_tbl_t cmd_list[] = {
        {
            "reset",      1,   do_reset,
            "reset        - Software Reset\n"
        },
        {
            "conn",       3,   do_conn,
            "conn         - start connect\n"
        },
        {
            "setbaud",    2,   do_set_baud,
            "setbaud      - set serial baudrate\n"
        },
        {
            "connstate",  1,   do_conn_state,
            "connstate    - get conn state\n"
        },
        {
            "setmac",     2,   do_set_mac,
            "setmac       - set mac address\n"
        },
        {
            "getmac",     1,   do_get_mac,
            "getmac       - get mac address\n"
        },
        {
            "getver",     1,   do_get_ver,
            "getver       - get version\n"
        },
        {
            "getver5856", 1,   do_get_ver_5856,
            "getver5856   - get 5856 version\n"
        },
        {
            "ftest5856",  1,   do_factory_test_5856,
            "ftest5856    - 5856 factory test\n"
        },
        {
            "disconn",    1,   do_disconn,
            "disconn      - start disconnect\n"
        },
        {
            "wuserdata",  2,   do_write_usedata,
            "wusedate     - write user data\n"
        },
        {
            "ruserdata",  1,   do_read_usedata,
            "ruserdate     - read user data\n"
        },
    };
    i = sizeof(cmd_list) / sizeof(cmd_tbl_t);

    while (i--) {
        if (0 != console_cmd_add(&cmd_list[i])) {
            printf("Add cmd failed\r\n");
        }
    }
}

int factory_test() {
    unsigned int flag = (*((volatile unsigned int*)(0x40001024UL)));

    //printf("flag = %x\r\n", flag);
    if ((flag & (0x01UL << 29)) == 0) {
        return 0;
    }

    op_mode = 1;
    console_init();
    add_cmd();

    while (1);

    //return 0;
}
#endif
