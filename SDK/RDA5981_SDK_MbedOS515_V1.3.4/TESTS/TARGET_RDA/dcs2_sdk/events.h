// Copyright (2016) Baidu Inc. All rights reserved.

#ifndef BAIDU_IOT_TINYDU_DEMO_EVENTS_H
#define BAIDU_IOT_TINYDU_DEMO_EVENTS_H

#include <mbed.h>

namespace duer {

enum Events {
    EVT_KEY_REC_PRESS,
    EVT_KEY_REC_RELEASE,
    EVT_KEY_PAUSE,
    EVT_KEY_TEST,
    EVT_TOTAL
};

#ifdef MBED_HEAP_STATS_ENABLED
extern void memory_statistics(const char* tag);
#define MEMORY_STATISTICS(...)      memory_statistics(__VA_ARGS__)
#else
#define MEMORY_STATISTICS(...)
#endif

typedef mbed::Callback<void ()> event_handle_func;

void event_set_handler(uint32_t evt_id, event_handle_func handler);

template<typename T, typename M>
void event_set_handler(uint32_t evt_id, T* obj, M method) {
    event_set_handler(evt_id, event_handle_func(obj, method));
}

void event_trigger(uint32_t evt_id);

void event_loop();

} // namespace duer

#endif // BAIDU_IOT_TINYDU_DEMO_EVENTS_H
