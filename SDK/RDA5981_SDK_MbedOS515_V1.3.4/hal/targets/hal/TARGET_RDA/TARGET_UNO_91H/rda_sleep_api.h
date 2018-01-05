/* mbed Microcontroller Library
 * Copyright (c) 2006-2013 ARM Limited
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
#ifndef RDA_SLEEP_API_H
#define RDA_SLEEP_API_H

#if DEVICE_SLEEP

typedef enum {
    SLEEP_NORMAL    = 0,
    SLEEP_DEEP      = 1,
    SLEEP_LEVEL_NUM
} SLEEP_LEVEL_T;

typedef enum {
    WAKESRC_UART    = (0x01UL << 0),
    WAKESRC_GPIO    = (0x01UL << 1)
} WAKEUP_SOURCE_T;

typedef struct {
    void (*set_slp_mode) (unsigned char is_slp);
    int  (*is_slp_allow) (void);
    void (*indic_wakeup) (void);
} sleep_entry_t;

typedef void (*sys_wakeup_handler)(void);

#ifdef __cplusplus
extern "C" {
#endif

void sleep_entry_register(sleep_entry_t *entry);
void sleep_set_level(SLEEP_LEVEL_T level);
void sleep_management(void);

void user_sleep_init(void);
void user_sleep_allow(int yes);
void user_sleep_wakesrc_set(unsigned int flag, unsigned int arg);

#ifdef __cplusplus
}
#endif

#endif

#endif

