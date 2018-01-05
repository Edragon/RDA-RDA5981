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
#include "sleep_api.h"
#include "cmsis.h"
#include "mbed_interface.h"
#include "rda_sleep_api.h"

void sleep(void)
{
#if (DEVICE_SEMIHOST == 1)
    // ensure debug is disconnected
    mbed_interface_disconnect();
#endif

    // process sleep events
    sleep_management();
}

/*
* The mbed UNO_91H does not support the deepsleep mode
* as a debugger is connected to it (the mbed interface).
*
* We treat a deepsleep() as a normal sleep().
*/
void deepsleep(void)
{
#if (DEVICE_SEMIHOST == 1)
    // ensure debug is disconnected
    mbed_interface_disconnect();
#endif

    // set to sleep
    sleep();
}

