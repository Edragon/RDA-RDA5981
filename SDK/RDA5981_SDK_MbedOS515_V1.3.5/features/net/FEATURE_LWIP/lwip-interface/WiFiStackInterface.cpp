/* LWIP implementation of NetworkInterfaceAPI
 * Copyright (c) 2015 ARM Limited
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#include "WiFiStackInterface.h"
#include "lwip_stack.h"
#if defined(TARGET_UNO_91H)
#include "inet.h"

//#define WIFISTACK_DEBUG
#ifdef WIFISTACK_DEBUG
#define WIFISTACK_PRINT(fmt, ...) do {\
            printf(fmt, ##__VA_ARGS__);\
    } while (0)
#else
#define WIFISTACK_PRINT(fmt, ...)
#endif

#define SCAN_TIMES    5
#define RECONN_TIMES  5

char mbed_ip_addr[4]    = {192, 168, 66, 1};
char mbed_msk_addr[4]   = {255, 255, 255, 0};
char mbed_gw_addr[4]    = {192, 168, 66, 1};
char mbed_dhcp_start[4] = {192, 168, 66, 100};
char mbed_dhcp_end[4] = {192, 168, 66, 255};

/* Interface implementation */
WiFiStackInterface::WiFiStackInterface()
    : _dhcp(true), _ip_address(), _netmask(), _gateway(), _ip_address_ap()
{
}

nsapi_error_t WiFiStackInterface::set_network(const char *ip_address, const char *netmask, const char *gateway)
{
    _dhcp = false;
    memcpy(_ip_address, ip_address ? ip_address : "", sizeof(_ip_address));
    memcpy(_netmask, netmask ? netmask : "", sizeof(_netmask));
    memcpy(_gateway, gateway ? gateway : "", sizeof(_gateway));
    return NSAPI_ERROR_OK;
}

nsapi_error_t WiFiStackInterface::set_network_ap(const char *ip_address, const char *netmask, const char *gateway,
                const char *dhcp_start, const char *dhcp_end)
{
    memcpy(_ip_address_ap, ip_address ? ip_address : "", sizeof(_ip_address_ap));
    memcpy(_netmask_ap, netmask ? netmask : "", sizeof(_netmask_ap));
    memcpy(_gateway_ap, gateway ? gateway : "", sizeof(_gateway_ap));
    memcpy(_dhcp_start, dhcp_start ? dhcp_start : "", sizeof(_dhcp_start));
    memcpy(_dhcp_end, dhcp_end ? dhcp_end : "", sizeof(_dhcp_end));
    return NSAPI_ERROR_OK;
}


nsapi_error_t WiFiStackInterface::set_dhcp(bool dhcp)
{
    _dhcp = dhcp;
    return NSAPI_ERROR_OK;
}

int WiFiStackInterface::scan(const char *ssid, const unsigned char channel)
{
    mbed_lwip_init(NULL);
    return rda5981_scan(ssid, strlen(ssid), channel);
}

int WiFiStackInterface::scan(const char *ssid, const unsigned char channel,
        unsigned char active, unsigned char scan_time)
{
    mbed_lwip_init(NULL);
    return rda5981_scan_v2(ssid, strlen(ssid), channel, active, scan_time);
}

int WiFiStackInterface::scan_result(rda5981_scan_result *bss_list, unsigned int num)
{
    if(bss_list == NULL)
        return 0;
    mbed_lwip_init(NULL);
    return rda5981_get_scan_result(bss_list, num);
}

int WiFiStackInterface::ap_join_info(rda5981_apsta_info *sta_list, unsigned int num)
{
    if(sta_list == NULL)
        return 0;
    return rda5981_get_ap_join_info(sta_list, num);
}


void WiFiStackInterface::set_msg_queue(void *queue)
{
    rda5981_set_main_queue(queue);
    return;
}

void WiFiStackInterface::init()
{
    mbed_lwip_init(NULL);
}

void WiFiStackInterface::set_sta_listen_interval(unsigned char interval)
{
    rda5981_set_sta_listen_interval(interval);
}

void WiFiStackInterface::set_sta_link_loss_time(unsigned char time)
{
    rda5981_set_sta_link_loss_time(time);
}

int WiFiStackInterface::connect(const char *ssid, const char *pass, const char *bssid, nsapi_security_t security, uint8_t channel)
{
    int ret = 0;
    int reconn=0, scan_times=0, find = 0;
    int scan_res = 0;
    rda5981_scan_result *bss_list = NULL;

    mbed_lwip_init(NULL);

    if (ssid == NULL) {
        WIFISTACK_PRINT("ERROR! ssid is NULL!\r\n");
        ret = -1;
        goto exit;
    } else {
        strcpy(SSID, ssid);
        strcpy(passwd, pass);
        if(bssid != NULL)
            memcpy(BSSID, bssid, NSAPI_MAC_BYTES);
        else
            memset(BSSID, 0, NSAPI_MAC_BYTES);
    }

    //scan
    rda5981_del_scan_all_result();
    while (scan_times++ < SCAN_TIMES)
        scan_res = rda5981_scan(SSID, strlen(SSID), channel);

    find = rda5981_check_scan_result(ssid, bssid, channel);
    if(find != 0){
        WIFISTACK_PRINT("Can't find the special AP\r\n");
        ret = -1;
        goto exit;
    }

    //connect
    do{
        ret = mbed_lwip_bringup(SSID, passwd, bssid, _dhcp,
            _ip_address[0] ? _ip_address : 0,
            _netmask[0] ? _netmask : 0,
            _gateway[0] ? _gateway : 0);
        WIFISTACK_PRINT("bringup result ret %d\r\n", ret);

        if(ret != 0){
            memset(&_bss, 0, sizeof(_bss));
            mbed_lwip_bringdownlink();
            Thread::wait(1000);
            if(++reconn > RECONN_TIMES)
                break;
        }
    }while(ret != 0);

    if(ret == 0){
        get_ip_address();
        get_netmask();
        get_gateway();
        rda5981_get_joined_AP(&_bss);
        WIFISTACK_PRINT("IP: %s\r\n", _ip_address);
        WIFISTACK_PRINT("SN: %s\r\n", _netmask);
        WIFISTACK_PRINT("GW: %s\r\n", _gateway);
    }
exit:
    if (bss_list)
        free(bss_list);

    return ret;
}

int WiFiStackInterface::reconnect()
{
    int ret = 0;
    int scan_times=0, find = 0;
    int scan_res = 0;
    rda5981_scan_result *bss_list = NULL;

    //scan
    rda5981_del_scan_all_result();
    while (scan_times++ < SCAN_TIMES)
        scan_res = rda5981_scan(SSID, strlen(SSID), 0);

    find = rda5981_check_scan_result(SSID, BSSID, 0);
    if(find != 0){
        WIFISTACK_PRINT("Can't find the special AP\r\n");
        ret = -1;
        goto exit;
    }

    //connect
    ret = mbed_lwip_bringup(SSID, passwd, BSSID, _dhcp,
        _ip_address[0] ? _ip_address : 0,
        _netmask[0] ? _netmask : 0,
        _gateway[0] ? _gateway : 0);
    WIFISTACK_PRINT("bringup result ret %d\r\n", ret);

    if(ret != 0){
        memset(&_bss, 0, sizeof(_bss));
        mbed_lwip_bringdownlink();
    }else{
        get_ip_address();
        get_netmask();
        get_gateway();
        rda5981_get_joined_AP(&_bss);
        WIFISTACK_PRINT("IP: %s\r\n", _ip_address);
        WIFISTACK_PRINT("SN: %s\r\n", _netmask);
        WIFISTACK_PRINT("GW: %s\r\n", _gateway);
    }

exit:
    if (bss_list)
        free(bss_list);

    return ret;
}

nsapi_error_t WiFiStackInterface::connect()
{
    return 0;
}

int WiFiStackInterface::start_ap(const char *ssid, const char *pass, int channel)
{
    return start_ap(ssid, pass, channel, 0);
}

int WiFiStackInterface::start_ap(const char *ssid, const char *pass, int channel, char mode)
{
    if(_ip_address_ap[0] == 0){
        memcpy(_ip_address_ap, inet_ntoa(*(unsigned int *)mbed_ip_addr), NSAPI_IPv4_SIZE);
        memcpy(_netmask_ap, inet_ntoa(*(unsigned int *)mbed_msk_addr), NSAPI_IPv4_SIZE);
        memcpy(_gateway_ap, inet_ntoa(*(unsigned int *)mbed_gw_addr), NSAPI_IPv4_SIZE);
        memcpy(_dhcp_start, inet_ntoa(*(unsigned int *)mbed_dhcp_start), NSAPI_IPv4_SIZE);
        memcpy(_dhcp_end, inet_ntoa(*(unsigned int *)mbed_dhcp_end), NSAPI_IPv4_SIZE);
        return mbed_lwip_startap_v2(ssid, pass, mbed_ip_addr, mbed_msk_addr, mbed_gw_addr, mbed_dhcp_start, mbed_dhcp_end, channel, mode);
    }else{
        u32_t ip = inet_addr(_ip_address_ap);
        u32_t mask = inet_addr(_netmask_ap);
        u32_t gw = inet_addr(_gateway_ap);
        u32_t dhcps = inet_addr(_dhcp_start);
        u32_t dhcpe = inet_addr(_dhcp_end);
        return mbed_lwip_startap_v2(ssid, pass, (const char *)&ip, (const char *)&mask, (const char *)&gw, (const char *)&dhcps, (const char *)&dhcpe, channel, mode);
    }
}

int WiFiStackInterface::stop_ap(unsigned char state)//0 ~ active stop, 1 ~ inactive stop
{
    memset(_ip_address_ap, 0, sizeof(_ip_address_ap));
    memset(_netmask_ap, 0, sizeof(_netmask_ap));
    memset(_gateway_ap, 0, sizeof(_gateway_ap));
    memset(_dhcp_start, 0, sizeof(_dhcp_start));
    memset(_dhcp_end, 0, sizeof(_dhcp_end));
    return mbed_lwip_stopap(state);
}


int WiFiStackInterface::disconnect()
{
    memset(&_bss, 0, sizeof(_bss));
    memset(_ip_address, 0, IPADDR_STRLEN_MAX);
    memset(_netmask, 0, NSAPI_IPv4_SIZE);
    memset(_gateway, 0, NSAPI_IPv4_SIZE);
    return mbed_lwip_bringdown();
}
/*
const char *WiFiStackInterface::get_sm_host_ip_address()
{
    return lwip_get_sm_host_ip_address();
}
const char *WiFiStackInterface::get_sm_ssid()
{
    return lwip_get_sm_ssid();
}
const char *WiFiStackInterface::get_sm_passwd()
{
    return lwip_get_sm_passwd();
}
*/

const char *WiFiStackInterface::get_ip_address_ap()
{
    return _ip_address_ap;
}

const char *WiFiStackInterface::get_netmask_ap()
{
    return _netmask_ap;
}

const char *WiFiStackInterface::get_gateway_ap()
{
    return _gateway_ap;
}

const char *WiFiStackInterface::get_dhcp_start_ap()
{
    return _dhcp_start;
}

const char *WiFiStackInterface::get_dhcp_end_ap()
{
    return _dhcp_end;
}

const char *WiFiStackInterface::get_ip_address()
{
    if (mbed_lwip_get_ip_address(_ip_address, sizeof _ip_address)) {
        return _ip_address;
    }

    return NULL;
}

const char *WiFiStackInterface::get_netmask()
{
    if (mbed_lwip_get_netmask(_netmask, sizeof _netmask)) {
        return _netmask;
    }

    return 0;
}

const char *WiFiStackInterface::get_gateway()
{
    if (mbed_lwip_get_gateway(_gateway, sizeof _gateway)) {
        return _gateway;
    }

    return 0;
}


const char *WiFiStackInterface::get_mac_address()
{
    return mbed_lwip_get_mac_address(0);
}

const char *WiFiStackInterface::get_mac_address_ap()
{
    return mbed_lwip_get_mac_address(1);
}

const char *WiFiStackInterface::get_BSSID()
{
    return _bss.BSSID;
}

int8_t WiFiStackInterface::get_channel()
{
    return _bss.channel;
}

int8_t WiFiStackInterface::get_rssi()
{
    return _bss.RSSI;
}

void WiFiStackInterface::update_rssi()
{
    int ret;
    rda5981_scan_result bss;
    //rda5981_scan(_bss.SSID, _bss.SSID_len, _bss.channel);
    //ret = rda5981_get_scan_result_name(&bss, (unsigned char*)_bss.SSID);
    ret = rda5981_get_joined_AP(&bss);
    if(!ret)
        _bss.RSSI = bss.RSSI;
    else
        _bss.RSSI = -128;
}

const char *WiFiStackInterface::get_ssid()
{
    return _bss.SSID;
}

NetworkStack * WiFiStackInterface::get_stack()
{
    return nsapi_create_stack(&lwip_stack);
}
#endif
