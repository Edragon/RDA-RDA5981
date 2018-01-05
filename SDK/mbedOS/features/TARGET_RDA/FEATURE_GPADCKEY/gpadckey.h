/*
 * File : gpadckey.h
 * Dscr : Headers for GPADC Key pad.
 */

#ifndef GPADC_KEY_H
#define GPADC_KEY_H

//#include "platform.h"

#include "rda_gpadc_api.h"
#include "ticker_api.h"
#include "Callback.h"
#include "critical.h"

//#define GPADC_KEY_DEBUG
#define USING_RT_TIME

namespace mbed {

#define KEY_SHIFT   (4)
#define PAD_NUM     (2)
#define PAD_KEY_NUM (6)

/* Enums */
typedef enum {
    KEY_A0 = ((GPADC0_0 << KEY_SHIFT) | 0),
    KEY_A1 = ((GPADC0_0 << KEY_SHIFT) | 1),
    KEY_A2 = ((GPADC0_0 << KEY_SHIFT) | 2),
    KEY_A3 = ((GPADC0_0 << KEY_SHIFT) | 3),
    KEY_A4 = ((GPADC0_0 << KEY_SHIFT) | 4),
    KEY_A5 = ((GPADC0_0 << KEY_SHIFT) | 5),
    KEY_B0 = ((GPADC0_1 << KEY_SHIFT) | 0),
    KEY_B1 = ((GPADC0_1 << KEY_SHIFT) | 1),
    KEY_B2 = ((GPADC0_1 << KEY_SHIFT) | 2),
    KEY_B3 = ((GPADC0_1 << KEY_SHIFT) | 3),
    KEY_B4 = ((GPADC0_1 << KEY_SHIFT) | 4),
    KEY_B5 = ((GPADC0_1 << KEY_SHIFT) | 5)
} KeyName;

class GpadcKey
{
public:
    GpadcKey(KeyName key);
    virtual ~GpadcKey();

    void rise(Callback<void()> func);

    template<typename T, typename M>
    void rise(T *obj, M method) {
        core_util_critical_section_enter();
        rise(Callback<void()>(obj, method));
        core_util_critical_section_exit();
    }

    void fall(Callback<void()> func);

    template<typename T, typename M>
    void fall(T *obj, M method) {
        core_util_critical_section_enter();
        fall(Callback<void()>(obj, method));
        core_util_critical_section_exit();
    }

    static void _irq_handler(uint8_t ch, uint16_t val, gpadc_event event);
    static uint8_t _get_key_idx(uint16_t val) {
        const uint16_t key_val_map[PAD_KEY_NUM] = {
            0x0000U, //Key 1, must be 0x0000U
            0x0067U, //Key 2
            0x00D9U, //Key 3
            0x0141U, //Key 4
            0x01ABU, //Key 5
            0x0211U  //Key 6
        };
        uint8_t idx;

        if(0x03FFU > val) {
            for(idx = 0; idx < (PAD_KEY_NUM - 1); idx++) {
                if((key_val_map[idx] <= val) && (key_val_map[idx + 1] > val)) {
                    if((key_val_map[idx + 1] - val) < (val - key_val_map[idx]))
                        idx++;
                    break;
                }
            }
        } else {
            idx = 0xFFU;
        }
        return(idx);
    }

private:
    static uint32_t obj_id[PAD_NUM][PAD_KEY_NUM];
    static gpadc_t gpadc[PAD_NUM];
    static uint8_t is_initialized[PAD_NUM];
    static uint8_t last_key_idx_calib[PAD_NUM];
    static uint32_t key_fall_ts;
    static uint32_t key_rise_ts;
#if defined(GPADC_KEY_DEBUG)
    static gpadc_event last_key_event[PAD_NUM];
#endif /* GPADC_KEY_DEBUG */
#if !defined(USING_RT_TIME)
    static const ticker_data_t *ticker_ptr;
#endif /* !USING_RT_TIME */
    uint8_t key_idx;
    uint8_t pad_idx;
    PinName pin;
    Callback<void()> _rise;
    Callback<void()> _fall;
};

} // namespace mbed

#endif
