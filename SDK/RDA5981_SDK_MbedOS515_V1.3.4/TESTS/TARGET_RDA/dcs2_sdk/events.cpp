#include "events.h"
#include "mbed.h"
#include "mbed_stats.h"

namespace duer {

#ifdef MBED_HEAP_STATS_ENABLED
void memory_statistics(const char* tag) {
    if (tag == NULL) {
        tag = "main";
    }

    mbed_stats_heap_t current;
    mbed_stats_heap_get(&current);
    printf("[%s] !!!MEMORY!!! current_size: %d, max_size: %d, total_size: %d, alloc_cnt: %d, "
        "alloc_fail_cnt: %d\n", tag, current.current_size, current.max_size, current.total_size,
        current.alloc_cnt, current.alloc_fail_cnt);
}
#endif

static rtos::Queue<int, 5> g_message_q;
static event_handle_func   g_events_handler[EVT_TOTAL];

void event_set_handler(uint32_t evt_id, event_handle_func handler) {
    if (evt_id < EVT_TOTAL) {
        g_events_handler[evt_id] = handler;
    }
}

void event_trigger(uint32_t evt_id) {
    g_message_q.put((int*)evt_id);
}

static void event_loop_run() {
    osEvent evt;

    do {
        evt = g_message_q.get();

        if (evt.status != osEventMessage) {
            continue;
        }

        uint32_t evt_id = evt.value.v;

        if (evt_id < EVT_TOTAL) {
            g_events_handler[evt_id]();
        }
    } while (true);
}

void event_loop() {
#ifdef __BAIDU_EVENT_SINGLE_TASK__
    Thread th(osPriorityHigh);
    th.start(event_loop_run);
    Thread::wait(osWaitForever);
#else
    event_loop_run();
#endif
}

} // namespace duer
