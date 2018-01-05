// Copyright (2017) Baidu Inc. All rights reserved.
/**
 * File: duerapp_alarm.cpp
 * Auth: Gang Chen(chengang12@baidu.com)
 * Desc: Duer alarm functions.
 */

#include "duerapp_alarm.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <list.h>

#include "mbed.h"
#include "heap_monitor.h"
#include "baidu_alarm.h"
#include "baidu_ca_ntp.h"
#include "baidu_ca_types.h"
#include "duer_log.h"
#include "baidu_media_manager.h"
#include "baidu_iot_mutex.h"
#include "baidu_measure_stack.h"

namespace duer {

typedef struct duerapp_alarm_node_{
    int id;
    int time;
    char *token;
    char *url;
    baidu_timer_t handle;
}duerapp_alarm_node;

typedef struct _duer_alarm_list {
    struct _duer_alarm_list *next;
    duerapp_alarm_node *data;
} duer_alarm_list_t;

static duer_alarm_list_t s_alarm_list;
static const int ALARM_QUEUE_SIZE = 5;
static const int ALARM_TREAD_STACK_SIZE = 1024 * 4;
static rtos::Queue<duerapp_alarm_node, ALARM_QUEUE_SIZE> s_message_q;
static rtos::Thread s_alarm_thread(osPriorityNormal, ALARM_TREAD_STACK_SIZE);
static iot_mutex_t s_alarm_mutex;

static bca_errcode_e duer_alarm_list_push(duerapp_alarm_node *data)
{
    bca_errcode_e rt = BCA_NO_ERR;
    duer_alarm_list_t *new_node = NULL;
    duer_alarm_list_t *tail = &s_alarm_list;

    new_node = (duer_alarm_list_t *)MALLOC(sizeof(duer_alarm_list_t), ALARM);
    if (!new_node) {
        DUER_LOGE("Memory too low");
        rt = BCA_ERR_INTERNAL;
        goto error_out;
    }
    new_node->next = NULL;
    new_node->data = data;

    while (tail->next) {
        tail = tail->next;
    }

    tail->next = new_node;

error_out:
    return rt;
}

static bca_errcode_e duer_alarm_list_remove(duerapp_alarm_node *data)
{
    duer_alarm_list_t *pre = &s_alarm_list;
    duer_alarm_list_t *cur = NULL;
    bca_errcode_e rt = BCA_ERR_INTERNAL;

    while (pre->next) {
        cur = pre->next;
        if (cur->data == data) {
            pre->next = cur->next;
            FREE(cur);
            rt = BCA_NO_ERR;
            break;
        }
        pre = pre->next;
    }

    return rt;
}

static duerapp_alarm_node *duer_create_alarm_node(const int id,
                                                  const int time,
                                                  const char *token,
                                                  const char *url,
                                                  baidu_timer_t handle)
{
    size_t len = 0;
    duerapp_alarm_node *alarm = NULL;

    alarm = (duerapp_alarm_node *)MALLOC(sizeof(duerapp_alarm_node), ALARM);
    if (!alarm) {
        goto error_out;
    }

    memset(alarm, 0, sizeof(duerapp_alarm_node));

    len = strlen(token) + 1;
    alarm->token = (char *)MALLOC(len, ALARM);
    if (!alarm->token) {
        goto error_out;
    }
    snprintf(alarm->token, len, "%s", token);

    len = strlen(url) + 1;
    alarm->url = (char *)MALLOC(len, ALARM);
    if (!alarm->url) {
        return NULL;
    }
    snprintf(alarm->url, len, "%s", url);

    alarm->id = id;
    alarm->time = time;
    alarm->handle = handle;

    return alarm;

error_out:
    DUER_LOGE("Memory too low");
    if (alarm) {
        if (alarm->token) {
            FREE(alarm->token);
        }

        if (alarm->url) {
            FREE(alarm->url);
        }

        FREE(alarm);
    }

    return NULL;
}

static void duer_free_alarm_node(duerapp_alarm_node *alarm)
{
    if (alarm) {
        if (alarm->token) {
            FREE(alarm->token);
            alarm->token = NULL;
        }

        if (alarm->url) {
            FREE(alarm->url);
            alarm->url = NULL;
        }

        if (alarm->handle) {
            baidu_timer_destroy(alarm->handle);
            alarm->handle = NULL;
        }

        FREE(alarm);
    }
}

static void duer_alarm_callback(void *param)
{
    duerapp_alarm_node *node = (duerapp_alarm_node *)param;

    DUER_LOGI("alarm started: token: %s, url: %s\n", node->token, node->url);

    duer_report_alarm_event(0, node->token, ALERT_START);
    // play url
    MediaManager::instance().play_url(node->url, false, true);
    iot_mutex_lock(s_alarm_mutex, 0);
    duer_alarm_list_remove(node);
    iot_mutex_unlock(s_alarm_mutex);
    duer_free_alarm_node(node);
}

static int duer_alarm_set(const int id, const char *token, const char *url, const int time)
{
    int rs = BCA_NO_ERR;
    duerapp_alarm_node* alarm = NULL;

    DUER_LOGI("set alarm: scheduled_time: %d, token: %s\n", time, token);

    alarm = duer_create_alarm_node(id, time, token, url, NULL);
    if (!alarm) {
        duer_report_alarm_event(id, token, SET_ALERT_FAIL);
        rs = BCA_ERR_INTERNAL;
    } else {
        // create alarm in duer_alarm_thread and return immediately
        s_message_q.put(alarm, 1);
    }

    return rs;
}

static duerapp_alarm_node *duer_find_target_alarm(const char *token)
{
    duer_alarm_list_t *node = s_alarm_list.next;

    while (node) {
        if (node->data) {
            if (strcmp(token, node->data->token) == 0) {
                return node->data;
            }
        }

        node = node->next;
    }

    return NULL;
}

/**
 * We use ntp to get current time, it might spend too long time and block the thread,
 * hence we use a new thread to create alarm.
 */
static void duer_alarm_thread()
{
    DuerTime cur_time;
    size_t delay = 0;
    int rs = BCA_NO_ERR;
    duerapp_alarm_node* alarm = NULL;

    while (1) {
        osEvent evt = s_message_q.get();

        if (evt.status != osEventMessage) {
            continue;
        }

        alarm = (duerapp_alarm_node*)evt.value.p;

        rs = bca_ntp_client(NULL, 0, &cur_time);
        if (rs < 0) {
            DUER_LOGE("Failed to get NTP time\n");
            duer_report_alarm_event(alarm->id, alarm->token, SET_ALERT_FAIL);
            continue;
        }

        if (alarm->time <= cur_time.sec) {
            DUER_LOGE("The alarm is expired\n");
            duer_report_alarm_event(alarm->id, alarm->token, SET_ALERT_FAIL);
            continue;
        }

        delay = (alarm->time - cur_time.sec) * 1000 + cur_time.usec / 1000;
        alarm->handle = baidu_timer_create(duer_alarm_callback,
                                           NULL,
                                           TIMER_TYPE_ONCE,
                                           alarm,
                                           delay);
        if (!alarm->handle) {
            DUER_LOGE("Failed to create timer\n");
            duer_report_alarm_event(alarm->id, alarm->token, SET_ALERT_FAIL);
            continue;
        }

        rs = baidu_timer_start(alarm->handle);
        if (rs != TIMER_RET_OK) {
            DUER_LOGE("Failed to start timer\n");
            duer_report_alarm_event(alarm->id, alarm->token, SET_ALERT_FAIL);
            continue;
        }

        /*
         * The alarms is storaged in the ram, hence the alarms will be lost after close the device.
         * You could stoage them into flash or sd card, and restore them after restart the device
         * according to your request.
         */
        iot_mutex_lock(s_alarm_mutex, 0);
        duer_alarm_list_push(alarm);
        iot_mutex_unlock(s_alarm_mutex);
        duer_report_alarm_event(alarm->id, alarm->token, SET_ALERT_SUCCESS);
    }
}

static int duer_alarm_delete(const int id, const char *token)
{
    duerapp_alarm_node *target_alarm = NULL;

    DUER_LOGI("delete alarm: token %s", token);

    iot_mutex_lock(s_alarm_mutex, 0);

    target_alarm = duer_find_target_alarm(token);
    if (!target_alarm) {
        DUER_LOGE("Cannot find the target alarm\n");
        iot_mutex_unlock(s_alarm_mutex);
        duer_report_alarm_event(id, token, DELETE_ALERT_FAIL);
        return BCA_ERR_INTERNAL;
    }

    duer_alarm_list_remove(target_alarm);

    iot_mutex_unlock(s_alarm_mutex);

    duer_free_alarm_node(target_alarm);
    duer_report_alarm_event(id, token, DELETE_ALERT_SUCCESS);

    return BCA_NO_ERR;
}

void duer_init_alarm()
{
    static bool is_first_time = true;

    /**
     * The init function might be called sevaral times when ca re-connect,
     * but some operations only need to be done once.
     */
    if (is_first_time) {
        is_first_time = false;
        s_alarm_mutex = iot_mutex_create();
        s_alarm_thread.start(duer_alarm_thread);
#ifdef BAIDU_STACK_MONITOR
        register_thread(&s_alarm_thread, "duer_alarm_thread");
#endif
    }

    duer_alarm_initialize(duer_alarm_set, duer_alarm_delete);
}

}

