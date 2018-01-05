/*
 * File : gpadckey.c
 * Dscr : Functions for GPADC Key pad.
 */
#include "gpadckey.h"
#include "us_ticker_api.h"
#include "rda_ccfg_api.h"

#if defined(GPADC_KEY_DEBUG)
#include "mbed_interface.h"
#endif /* GPADC_KEY_DEBUG */
#if defined(USING_RT_TIME)
extern "C" {
    extern unsigned int rt_time_get(void);
}
#endif /* USING_RT_TIME */

namespace mbed {

#define MIN_KEY_HOLDTIME_MS         (50)
#define MIN_KEY_RELEASETIME_MS      (100)

/* Static variables */
uint32_t GpadcKey::obj_id[PAD_NUM][PAD_KEY_NUM] = {0};
gpadc_t GpadcKey::gpadc[PAD_NUM] = {{(GPADCName)0, NC, 0}, {(GPADCName)0, NC, 0}};
uint8_t GpadcKey::is_initialized[PAD_NUM] = {0};
uint8_t GpadcKey::last_key_idx_calib[PAD_NUM] = {0};
uint32_t GpadcKey::key_fall_ts = 0;
uint32_t GpadcKey::key_rise_ts = 0;
#if defined(GPADC_KEY_DEBUG)
gpadc_event GpadcKey::last_key_event[PAD_NUM] = {GP_NONE, GP_NONE};
#endif /* GPADC_KEY_DEBUG */
#if !defined(USING_RT_TIME)
const ticker_data_t * GpadcKey::ticker_ptr = get_us_ticker_data();
#endif /* !USING_RT_TIME */

/* Constructor */
GpadcKey::GpadcKey(KeyName key) : key_idx(),
                                  pad_idx(),
                                  pin(),
                                  _rise(),
                                  _fall()
{
    key_idx = (uint8_t)((uint8_t)key & ((0x01U << KEY_SHIFT) - 0x01U));
    MBED_ASSERT(key_idx < PAD_KEY_NUM);
    pad_idx = (uint8_t)((uint8_t)key >> KEY_SHIFT & 0x01U);
    MBED_ASSERT(pad_idx < PAD_NUM);
    GpadcKey::obj_id[pad_idx][key_idx] = (uint32_t)this;
    switch((GPADCName)pad_idx)
    {
    case GPADC0_0:
        pin = PB_6;
        break;
    case GPADC0_1:
        if (rda_ccfg_hwver() >= 4) {
            pin = PB_8;
        } else {
            pin = PB_7;
        }
        break;
    default:
        // do nothing
        break;
    }

    if(0 == GpadcKey::is_initialized[pad_idx]) {
        rda_gpadc_init(&(GpadcKey::gpadc[pad_idx]), pin, (&GpadcKey::_irq_handler));
        GpadcKey::is_initialized[pad_idx] = 1;
    }
}

GpadcKey::~GpadcKey() {
    // No lock needed in the destructor
    rda_gpadc_free(&(GpadcKey::gpadc[pad_idx]));
}

void GpadcKey::rise(Callback<void()> func)
{
    core_util_critical_section_enter();
    if(func) {
        _rise.attach(func);
        rda_gpadc_set(&(GpadcKey::gpadc[pad_idx]), GP_RISE, 1);
    } else {
        _rise.attach(NULL);
        rda_gpadc_set(&(GpadcKey::gpadc[pad_idx]), GP_RISE, 0);
    }
    core_util_critical_section_exit();
}

void GpadcKey::fall(Callback<void()> func)
{
    core_util_critical_section_enter();
    if(func) {
        _fall.attach(func);
        rda_gpadc_set(&(GpadcKey::gpadc[pad_idx]), GP_FALL, 1);
    } else {
        _fall.attach(NULL);
        rda_gpadc_set(&(GpadcKey::gpadc[pad_idx]), GP_FALL, 0);
    }
    core_util_critical_section_exit();
}

void GpadcKey::_irq_handler(uint8_t ch, uint16_t val, gpadc_event event)
{
    uint8_t idx = GpadcKey::_get_key_idx(val);
    GpadcKey *handler;
    uint32_t cur_tmr;

#if !defined(USING_RT_TIME)
    cur_tmr = ticker_read(ticker_ptr) / 1000U;
#else  /* !USING_RT_TIME */
    cur_tmr = rt_time_get();
#endif /* !USING_RT_TIME */
    if(GpadcKey::key_fall_ts > GpadcKey::key_rise_ts) {
        /* last event is fall */
        if(GP_FALL == event) {
            uint32_t last_fall_ts = GpadcKey::key_fall_ts;
            GpadcKey::key_fall_ts = cur_tmr;
            if(GpadcKey::key_rise_ts) {
                return;
            } else if((cur_tmr - last_fall_ts) < (MIN_KEY_HOLDTIME_MS + MIN_KEY_RELEASETIME_MS)) {
                return;
            }
        } else if(GP_RISE == event) {
            if((cur_tmr - GpadcKey::key_fall_ts) < MIN_KEY_HOLDTIME_MS) {
                return;
            }
        }
    } else {
        /* last event is rise */
        if(GP_FALL == event) {
            if((cur_tmr - GpadcKey::key_rise_ts) < MIN_KEY_RELEASETIME_MS) {
                return;
            }
        } else if(GP_RISE == event) {
            uint32_t last_rise_ts = GpadcKey::key_rise_ts;
            GpadcKey::key_rise_ts = cur_tmr;
            if(GpadcKey::key_fall_ts) {
                return;
            } else if((cur_tmr - last_rise_ts) < (MIN_KEY_HOLDTIME_MS + MIN_KEY_RELEASETIME_MS)) {
                return;
            }
        }
    }
    if(GP_FALL == event) {
        GpadcKey::key_fall_ts= cur_tmr;
    } else if(GP_RISE == event) {
        GpadcKey::key_rise_ts= cur_tmr;
    }

    if(PAD_KEY_NUM > idx) {
        GpadcKey::last_key_idx_calib[ch] = idx;
    } else {
        idx = GpadcKey::last_key_idx_calib[ch];
    }
    handler = (GpadcKey*)(GpadcKey::obj_id[ch][idx]);

#if defined(GPADC_KEY_DEBUG)
    if((handler->_rise) && (handler->_fall) && (GpadcKey::last_key_event[ch] == event)) {
        mbed_error_printf("miss event: %d,val:%04X(%d),%d\r\n", cur_tmr, val, idx, (int)event);
    }
    GpadcKey::last_key_event[ch] = event;
#endif /* GPADC_KEY_DEBUG */
    if(handler) {
        switch (event) {
            case GP_RISE:
                handler->_rise.call();
                break;
            case GP_FALL:
                handler->_fall.call();
                break;
            case GP_NONE:
                break;
        }
    }
}

} // namespace mbed

