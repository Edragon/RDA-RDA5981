// Copyright (2016) Baidu Inc. All rights reserved.
/**
 * File: main.cpp
 * Desc: Sample code for startup DuerOS
 */

#include "mbed.h"
#include "baidu_media_manager.h"
#include "duer_app.h"
#include "events.h"
#include "duer_log.h"
#if defined(TARGET_UNO_91H)
#include "SDMMCFileSystem.h"
#include "WiFiStackInterface.h"
#include "factory_test.h"
#include "gpadckey.h"
#ifdef RDA_SMART_CONFIG
#include "smart_config.h"
#endif // RDA_SMART_CONFIG
#elif defined(TARGET_K64F)
#include "SDFileSystem.h"
#include "EthernetInterface.h"
#else
#error "Not supported"
#endif // TARGET_UNO_91H

#if defined(ENABLE_UPNP_RENDER)
#include "minirender.h"
#include "baidu_measure_stack.h"
#endif

#if defined(TARGET_UNO_91H)

GpadcKey key_up(KEY_A3);
GpadcKey key_down(KEY_A4);

// Initialize SD card
SDMMCFileSystem g_sd(GPIO_PIN9, GPIO_PIN0, GPIO_PIN3, GPIO_PIN7, GPIO_PIN12, GPIO_PIN13, "sd");

static WiFiStackInterface s_net_stack;
#else
SDFileSystem g_sd = SDFileSystem(D11, D12, D13, D10, "sd");
static EthernetInterface s_net_stack;
#endif // TARGET_UNO_91H

static unsigned char s_volume = 8;
const char* ip;
const char* mac;

int rda_ota_enable=0;

void* baidu_get_netstack_instance(void) {
    return (void*)&s_net_stack;
}

void voice_up() {
    if (s_volume < 15) {
        duer::MediaManager::instance().set_volume(++s_volume);
    }
}

void voice_down() {
    if (s_volume > 0) {
        duer::MediaManager::instance().set_volume(--s_volume);
    }
}

#if defined(ENABLE_UPNP_RENDER)
void test_upnp_render(void const *argument) {

    Thread::wait(5000);

    printf("### test upnp start\n");

    upnp_test(ip, mac);

    printf("### test upnp end\n");
}
#endif

// main() runs in its own thread in the OS
int main() {
#if defined(TARGET_UNO_91H)
    // Initialize RDA FLASH
    const unsigned int RDA_FLASH_SIZE     = 0x400000;   // Flash Size
    const unsigned int RDA_SYS_DATA_ADDR  = 0x18204000; // System Data Area, fixed size 4KB
    const unsigned int RDA_USER_DATA_ADDR = 0x18205000; // User Data Area start address
    const unsigned int RDA_USER_DATA_LENG = 0x3000;     // User Data Area Length

    rda5981_set_flash_size(RDA_FLASH_SIZE);
    rda5981_set_user_data_addr(RDA_SYS_DATA_ADDR, RDA_USER_DATA_ADDR, RDA_USER_DATA_LENG);

    // Test added by RDA
    factory_test();
#endif

    DUER_LOGI("\nEntry Tinydu Main>>>>\n");

    duer::MediaManager::instance().initialize();

#if defined(TARGET_UNO_91H)
    key_up.fall(&voice_up);
    key_down.fall(&voice_down);
#endif

    // Brings up the network interface
#ifdef RDA_SMART_CONFIG
    typedef void (*dummy_func)();
    mbed::GpadcKey key_erase = mbed::GpadcKey(KEY_A2);
    key_erase.fall((dummy_func)rda5981_flash_erase_sta_data);

    int ret = 0;
    ret = s_net_stack.scan(NULL, 0);//necessary

    char smartconfig_ssid[duer::RDA_SMARTCONFIG_SSID_LENGTH];
    char smartconfig_password[duer::RDA_SMARTCONFIG_PASSWORD_LENGTH];
    ret = duer::smart_config(smartconfig_ssid, smartconfig_password);
    if (ret == 0 && s_net_stack.connect(smartconfig_ssid, smartconfig_password) == 0) {
#elif defined(TARGET_UNO_91H)
    if (s_net_stack.connect(CUSTOM_SSID, CUSTOM_PASSWD) == 0) {
#else
    if (s_net_stack.connect() == 0) {
#endif // RDA_SMART_CONFIG
        ip = s_net_stack.get_ip_address();
        mac = s_net_stack.get_mac_address();
        DUER_LOGI("IP address is: %s", ip ? ip : "No IP");
        DUER_LOGI("MAC address is: %s", mac ? mac : "No MAC");
    } else {
        DUER_LOGE("Network initial failed....");
        Thread::wait(osWaitForever);
    }

    duer::SocketAdapter::set_network_interface(&s_net_stack);

    duer::MediaManager::instance().set_volume(s_volume);

    duer::DuerApp app;
    app.start();

#if defined(ENABLE_UPNP_RENDER)
    Thread thread(test_upnp_render, NULL, osPriorityNormal, 10*1024);
#if defined(BAIDU_STACK_MONITOR)
    register_thread(&thread, "DLNA");
#endif
#endif

    duer::event_loop();
}
