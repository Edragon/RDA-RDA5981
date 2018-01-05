// Copyright (2016) Baidu Inc. All rights reserved.

#ifndef BAIDU_IOT_TINYDU_DEMO_DUER_APP_H
#define BAIDU_IOT_TINYDU_DEMO_DUER_APP_H

#include "baidu_recorder_manager.h"
#include "baidu_ca_scheduler.h"

#if defined(TARGET_UNO_91H)
// TODO: Configure your AP
#ifndef CUSTOM_SSID
#define CUSTOM_SSID         ("bytetd-ap")
#endif // CUSTOM_SSID

#ifndef CUSTOM_PASSWD
#define CUSTOM_PASSWD       ("www.bytetd.com")
#endif // CUSTOM_PASSWD
#endif // TARGET_UNO_91H

namespace duer
{

#ifdef RDA_SMART_CONFIG
const int RDA_SMARTCONFIG_SSID_LENGTH = 32 + 1;
const int RDA_SMARTCONFIG_PASSWORD_LENGTH = 64 + 1;
int smart_config(char* smartconfig_ssid, char* smartconfig_password);
#endif // RDA_SMART_CONFIG

class DuerApp
{
public:
    enum Color {
        WHITE,
        YELLOW,
        PURPLE,
        RED,
        CYAN,
        GREEN,
        BLUE,
        OFF
    };

    DuerApp();

    ~DuerApp();

    void start();

    void stop();

    void talk_start();

    void talk_stop();

    void set_color(Color c);

    void restart();

    int get_topic_id() const;

    void pause_play();

    void test();

    void reset_delay();

private:
    void button_fall_handle();

    void button_rise_handle();

    void pause_button_fall_handle();

    void test_handle();

#if defined(TEST_BOARD)
    void send_timestamp();
#endif

    RecorderManager      _recorder;
    Recorder::IListener* _recorder_listener;

    duer::Scheduler::IOnEvent* _on_event;

#if !defined(TARGET_UNO_91H)
    mbed::BusOut                    _indicate;
#endif

    rtos::RtosTimer                 _timer;
    unsigned int                    _delay;

#if defined(TEST_BOARD)
    rtos::RtosTimer                 _send_ticker;
#endif
};

} // namespace duer

#endif // BAIDU_IOT_TINYDU_DEMO_DUER_APP_H
