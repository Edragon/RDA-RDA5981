// Copyright (2016) Baidu Inc. All rights reserved.
/**
 * File: duer_app.cpp
 * Desc: Demo for how to start Duer OS.
 */

#include "duer_app.h"
#include "baidu_ca_scheduler.h"
#include "baidu_ca_network_socket.h"
#include "baidu_media_manager.h"
#include "duer_log.h"
#include "device_controller.h"
#include "events.h"
#if defined(TARGET_UNO_91H)
#include "gpadckey.h"
#include "WiFiStackInterface.h"
#endif
#include "baidu_json.h"

#include "duer_play_command_manager.h"
#ifdef RDA_SMART_CONFIG
#include "smart_config.h"
#endif // RDA_SMART_CONFIG

#include "duerapp_alarm.h"
#include "heap_monitor.h"

namespace duer {

#if defined(TARGET_UNO_91H)
#define LED_RED LED1
#define LED_GREEN LED2
#define LED_BLUE LED3

static GpadcKey s_button(KEY_A0);
static GpadcKey s_pause_button(KEY_A1);
static GpadcKey s_test(KEY_A5);
#else
#define DUER_APP_RECORDER_BUTTON  SW2
static mbed::InterruptIn s_button = mbed::InterruptIn(DUER_APP_RECORDER_BUTTON);
#endif

static const char BAIDU_DEV_CONNECT_SUCCESS_PROMPT_FILE[] = "/sd/cloud_connected.mp3";
static const char BAIDU_DEV_DISCONNECTED_PROMPT_FILE[] = "/sd/cloud_disconnected.mp3";
static const char BAIDU_DEV_RECONNECTWIFI_PROMPT_FILE[] = "/sd/cloud_reconnectwifi.mp3";
static const char BAIDU_DEV_RESTART_DEVICE_PROMPT_FILE[] = "/sd/cloud_restartdevice.mp3";

static const unsigned int RECONN_DELAY_MIN = 2000;
// after reconnect wifi fail, the min delay to retry connect, should less than RECONN_DELAY_MAX
static const unsigned int NEW_RECONN_DELAY = 32000;
static const unsigned int RECONN_DELAY_MAX = 32001;

#ifdef RDA_SMART_CONFIG
int smart_config(char* smartconfig_ssid, char* smartconfig_password) {
    int ret = 0;
    char ssid[RDA_SMARTCONFIG_SSID_LENGTH];
    char password[RDA_SMARTCONFIG_PASSWORD_LENGTH];

    ret = rda5981_flash_read_sta_data(ssid, password);
    if (ret == 0 && strlen(ssid)){
        DUER_LOGI("get ssid from flash: ssid:%s, pass:%s", ssid, password);
    }else{
        ret = rda5981_getssid_from_smartconfig(ssid, password);
        if (ret != 0){
            DUER_LOGI("get ssid from smartconfig fail!");
            return -1;
        }else{
            DUER_LOGI("ssid:%s, password:%s", ssid, password);
            rda5981_flash_write_sta_data(ssid, password);
        }
    }

    memcpy(smartconfig_ssid, ssid, RDA_SMARTCONFIG_SSID_LENGTH);
    memcpy(smartconfig_password, password, RDA_SMARTCONFIG_PASSWORD_LENGTH);
    return 0;
}
#endif // RDA_SMART_CONFIG

/*
 * use this class to play main url after played front tone url
 */
class AppMediaPlayerListener : public MediaPlayerListener {
public:
    AppMediaPlayerListener() : _next_url(NULL), _play_next_url(false), _has_new_url(false) {
    }

    virtual ~AppMediaPlayerListener() {
        if (_next_url != NULL) {
            FREE(_next_url);
            _next_url = NULL;
        }
    }

    virtual int on_start() {
        if (_next_url) {
            _play_next_url = true;
        }
        _has_new_url = false;

        return 0;
    }

    virtual int on_stop() {
        _play_next_url = false;
        if (_next_url != NULL && !_has_new_url) {
            FREE(_next_url);
            _next_url = NULL;
        }

        return 0;
    }

    virtual int on_finish() {
        if (_play_next_url && _next_url != NULL) {
            MediaManager::instance().play_url(_next_url, false, false);
            FREE(_next_url);
            _next_url = NULL;
            _play_next_url = false;
        }
        return 0;
    }

    void set_next_url(const char* url) {
        if (url != NULL) {
            _has_new_url = true;
            if (_next_url != NULL) {
                FREE(_next_url);
            }

            int len = strlen(url) + 1;
            _next_url = (char*)MALLOC(len, APP);
            if (_next_url != NULL) {
                snprintf(_next_url, len, "%s", url);
            } else {
                DUER_LOGE("No free memory.");
            }
        }
    }

private:
    char* _next_url;
    bool  _play_next_url;
    bool  _has_new_url;
};

class SchedulerEventListener : public Scheduler::IOnEvent {
public:
    SchedulerEventListener(DuerApp* app) :
            _app(app) {
        Scheduler::instance().set_on_event_listener(this);
    }

    virtual ~SchedulerEventListener() {
    }

    virtual int on_start() {
        DUER_LOGI("SchedulerEventListener::on_start");

        MEMORY_STATISTICS("Scheduler::on_start");

        device_controller_init();

#ifdef ENABLE_ALARM
        duer_init_alarm();
#endif

        MediaManager::instance().register_listener(&_media_player_listener);

        event_set_handler(EVT_KEY_REC_PRESS, _app, &DuerApp::talk_start);
        event_set_handler(EVT_KEY_REC_RELEASE, _app, &DuerApp::talk_stop);
        event_set_handler(EVT_KEY_PAUSE, _app, &DuerApp::pause_play);
        event_set_handler(EVT_KEY_TEST, _app, &DuerApp::test);

        _app->set_color(DuerApp::CYAN);

        MediaManager::instance().play_local(BAIDU_DEV_CONNECT_SUCCESS_PROMPT_FILE);
        _app->reset_delay();
        return 0;
    }

    virtual int on_stop() {
        _app->set_color(DuerApp::PURPLE);

        MediaManager::instance().stop();
        _app->talk_stop();

        MediaManager::instance().unregister_listener(&_media_player_listener);

        event_set_handler(EVT_KEY_REC_PRESS, NULL);
        event_set_handler(EVT_KEY_REC_RELEASE, NULL);
        event_set_handler(EVT_KEY_PAUSE, NULL);
        event_set_handler(EVT_KEY_TEST, NULL);

        MEMORY_STATISTICS("Scheduler::on_stop");

        DUER_LOGI("SchedulerEventListener::on_stop");

        _app->restart();

        return 0;
    }

    virtual int on_action(const char* url, bool is_speech, bool is_continue_previous) {
        // DUER_LOGI("SchedulerEventListener::on_action: %s", url);
        // _app->set_color(DuerApp::BLUE);
        // MediaManager::instance().play_url(url, is_speech, is_continue_previous);

        return 0;
    }

    virtual int on_data(const char* data) {
        DUER_LOGV("SchedulerEventListener::on_data: %s", data);

        char* main_url = NULL;
        char* front_tone_url = NULL;
        bool is_continue_previous = true;
        bool is_speech = true;

        // get main_url and front_tone_url by parse data
        //
        baidu_json* value = baidu_json_Parse(data);
        baidu_json* url = baidu_json_GetObjectItem(value, "url");

        if (url != NULL && url->valuestring != NULL) {
            main_url = url->valuestring;

            baidu_json* payload = baidu_json_GetObjectItem(value, "payload");
            if (payload != NULL) {
                baidu_json* payload_url = baidu_json_GetObjectItem(payload, "url");

                if (payload_url != NULL && payload_url->valuestring != NULL) {
                    is_speech = false;
                    if (strcmp(main_url, payload_url->valuestring) != 0) {
                        DUER_LOGE("main url is not url of payload");
                        main_url = payload_url->valuestring;
                    }
                    DUER_LOGD("url is not speech!");
                }

                baidu_json* behavior = baidu_json_GetObjectItem(payload, "play_behavior");
                if (behavior == NULL) {
                    behavior = baidu_json_GetObjectItem(payload, "speak_behavior");
                }
                if (behavior != NULL && behavior->valuestring != NULL
                    && strstr(behavior->valuestring, "REPLACE_ALL") != NULL) {
                    is_continue_previous = false;
                    DUER_LOGD("play_behavior or speak_behavior is REPLACE_ALL!");
                }
            }

            // read url of speech, when url of payload is not null
            if (!is_speech) {
                baidu_json* speech = baidu_json_GetObjectItem(value, "speech");

                if (speech != NULL) {
                    baidu_json* speech_url = baidu_json_GetObjectItem(speech, "url");

                    if (speech_url != NULL && speech_url->valuestring != NULL) {
                        front_tone_url = speech_url->valuestring;
                    }
                }
            }
        }

        DUER_LOGI("front_tone_url: %s", front_tone_url);
        DUER_LOGI("main_url: %s", main_url);
        DUER_LOGI("is_speech = %d, is_continue_previous = %d", is_speech, is_continue_previous);
        _app->set_color(DuerApp::BLUE);
        if (front_tone_url != NULL) {
            _media_player_listener.set_next_url(main_url);
            MediaManager::instance().play_url(front_tone_url, true, false);
        } else {
            MediaManager::instance().play_url(main_url, is_speech, is_continue_previous);
        }

        baidu_json_Delete(value);

        return 0;
    }

private:
    DuerApp* _app;
    AppMediaPlayerListener _media_player_listener;
};

class RecorderListener : public Recorder::IListener {
public:
    RecorderListener(DuerApp* app) :
        _app(app),
        _start_send_data(false) {
    }

    virtual ~RecorderListener() {
    }

    virtual int on_start() {
        _app->set_color(DuerApp::RED);
        Scheduler::instance().clear_content();
        DUER_LOGI("RecorderObserver::on_start");
        MEMORY_STATISTICS("Recorder::on_start");
        return 0;
    }

    virtual int on_resume() {
        return 0;
    }

    virtual int on_data(const void* data, size_t size) {
        if (!_start_send_data) {
            _start_send_data = true;
        }

        DUER_LOGV("RecorderObserver::on_data: data = %p, size = %d", data, size);
        Scheduler::instance().send_content(data, size, false);
        return 0;
    }

    virtual int on_pause() {
        return 0;
    }

    virtual int on_stop() {
        if (_start_send_data) {
            Scheduler::instance().send_content(NULL, 0, false);
            _start_send_data = false;
        } else {
            MediaManager::instance().stop_completely();
        }

        MEMORY_STATISTICS("Recorder::on_stop");
        DUER_LOGI("RecorderObserver::on_stop");
        _app->set_color(DuerApp::GREEN);
        return 0;
    }

private:
    DuerApp* _app;
    bool _start_send_data;
};

DuerApp::DuerApp()
    : _recorder_listener(new RecorderListener(this))
    , _on_event(new SchedulerEventListener(this))
#if !defined(TARGET_UNO_91H)
    , _indicate(LED_BLUE, LED_GREEN, LED_RED)
#endif
    , _timer(this, &DuerApp::start, osTimerOnce)
    , _delay(RECONN_DELAY_MIN)
#if defined(TEST_BOARD)
    , _send_ticker(this, &DuerApp::send_timestamp, osTimerPeriodic)
#endif
{
    _recorder.set_listener(_recorder_listener);

#if !defined(TARGET_UNO_91H)
    _indicate = OFF;
#endif
}

DuerApp::~DuerApp() {
    delete _recorder_listener;
    delete _on_event;
}

void DuerApp::start() {
    Scheduler::instance().start();

    s_button.fall(this, &DuerApp::button_fall_handle);
    s_button.rise(this, &DuerApp::button_rise_handle);
#if defined(TARGET_UNO_91H)
    s_pause_button.fall(this, &DuerApp::pause_button_fall_handle);
    s_test.fall(this, &DuerApp::test_handle);
#endif
}

void DuerApp::stop() {
    s_button.fall(NULL);
    s_button.rise(NULL);
#if defined(TARGET_UNO_91H)
    s_pause_button.fall(NULL);
#endif
    Scheduler::instance().stop();
}

void DuerApp::reset_delay() {
    _delay = RECONN_DELAY_MIN;
}

void DuerApp::restart() {
    if (_delay > RECONN_DELAY_MAX) {
        // try to reconnect the wifi interface
        WiFiStackInterface* net_stack
                = (WiFiStackInterface*)SocketAdapter::get_network_interface();
        if (net_stack != NULL) {
            MediaManager::instance().play_local(BAIDU_DEV_RECONNECTWIFI_PROMPT_FILE);
            net_stack->disconnect();
#ifdef RDA_SMART_CONFIG
            int ret = 0;
            ret = net_stack->scan(NULL, 0);//necessary

            char smartconfig_ssid[RDA_SMARTCONFIG_SSID_LENGTH];
            char smartconfig_password[RDA_SMARTCONFIG_PASSWORD_LENGTH];
            ret = smart_config(smartconfig_ssid, smartconfig_password);
            if (ret == 0 && net_stack->connect(smartconfig_ssid, smartconfig_password) == 0) {
#elif defined(TARGET_UNO_91H)
            if (net_stack->connect(CUSTOM_SSID, CUSTOM_PASSWD) == 0) {
#else
            if (net_stack->connect() == 0) {
#endif // RDA_SMART_CONFIG
                const char* ip = net_stack->get_ip_address();
                const char* mac = net_stack->get_mac_address();
                DUER_LOGI("IP address is: %s", ip ? ip : "No IP");
                DUER_LOGI("MAC address is: %s", mac ? mac : "No MAC");
                _delay = RECONN_DELAY_MIN;
            } else {
                _delay = NEW_RECONN_DELAY;
                DUER_LOGNW("Network initial failed, retry...");
            }
        } else {
            MediaManager::instance().play_local(BAIDU_DEV_RESTART_DEVICE_PROMPT_FILE);
            DUER_LOGNE("no net_stack got, try restart the device!!");
        }
    }
    if (_delay < RECONN_DELAY_MAX) {
        _timer.start(_delay);
        _delay <<= 1;
    } else {
        // due the wifi reconnect strategy, here will not enter
        MediaManager::instance().play_local(BAIDU_DEV_DISCONNECTED_PROMPT_FILE);
    }
}

void DuerApp::talk_start() {
    MediaManager::instance().stop();
    _recorder.start();
}

void DuerApp::talk_stop() {
    _recorder.stop();
}

void DuerApp::set_color(Color c) {
#if !defined(TARGET_UNO_91H)
    _indicate = c;
#endif

    if (c == CYAN) {
        _delay = RECONN_DELAY_MIN;
#if defined(TEST_BOARD)
        _send_ticker.start(60 * 1000);//update interval to 1min
#endif
    } else if (c == PURPLE) {
#if defined(TEST_BOARD)
        _send_ticker.stop();
#endif
    }
}

void DuerApp::button_fall_handle() {
    event_trigger(EVT_KEY_REC_PRESS);
}

void DuerApp::button_rise_handle() {
    event_trigger(EVT_KEY_REC_RELEASE);
}

void DuerApp::pause_button_fall_handle() {
    event_trigger(EVT_KEY_PAUSE);
}

void DuerApp::test_handle() {
    event_trigger(EVT_KEY_TEST);
}

void DuerApp::pause_play() {
    MediaManager::instance().pause_or_resume();
}

void DuerApp::test()
{
    static int i = 0;
    switch (i % 3) {
    case 0:
        duer::PlayCommandManager::instance().next();
        break;
    case 1:
        duer::PlayCommandManager::instance().previous();
        break;
    case 2:
        duer::PlayCommandManager::instance().repeat();
        break;
    }
    i++;
}

#if defined(TEST_BOARD)
void DuerApp::send_timestamp() {
    Object data;
    data.putInt("time", us_ticker_read());
    Scheduler::instance().report(data);
}
#endif

} // namespace duer
