/* mbed USBHost Library
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

#ifndef USBHALHOST_H
#define USBHALHOST_H

#include "USBHostTypes.h"
#include "USBHostConf.h"
#if defined(TARGET_UNO_91H)
#include "USBHALHost_UNO_91H.h"
#endif

class USBHostHub;

/**
* USBHALHost class
*/
class USBHALHost {
protected:

    /**
    * Constructor
    * init variables and memory where will be stored HCCA, ED and TD
    */
    USBHALHost();

    /**
    * Initialize host controller. Enable USB interrupts. This part is not in the constructor because,
    * this function calls a virtual method if a device is already connected
    */
    void init();

    /**
    * reset the root hub
    */
    void resetRootHub();

    /**
    * return the value contained in the control HEAD ED register
    *
    * @returns address of the control Head ED
    */
    uint32_t controlHeadED();

    /**
    * return the value contained in the bulk HEAD ED register
    *
    * @returns address of the bulk head ED
    */
    uint32_t bulkHeadED();

    /**
    * return the value of the head interrupt ED contained in the HCCA
    *
    * @returns address of the head interrupt ED contained in the HCCA
    */
    uint32_t interruptHeadED();

    /**
    * Update the head ED for control transfers
    */
    void updateControlHeadED(uint32_t addr);

    /**
    * Update the head ED for bulk transfers
    */
    void updateBulkHeadED(uint32_t addr);

    /**
    * Update the head ED for interrupt transfers
    */
    void updateInterruptHeadED(uint32_t addr);

    /**
    * Enable List for the specified endpoint type
    *
    * @param type enable the list of ENDPOINT_TYPE type
    */
    void enableList(ENDPOINT_TYPE type);

    /**
    * Disable List for the specified endpoint type
    *
    * @param type disable the list of ENDPOINT_TYPE type
    */
    bool disableList(ENDPOINT_TYPE type);

    /**
    * Virtual method called when a device has been connected
    *
    * @param hub hub number of the device
    * @param port port number of the device
    * @param lowSpeed 1 if low speed, 0 otherwise
    * @param hub_parent reference to the hub where the device is connected (NULL if the hub parent is the root hub)
    */
    virtual void deviceConnected(int hub, int port, bool lowSpeed, USBHostHub * hub_parent = NULL) = 0;

    /**
    * Virtual method called when a device has been disconnected
    *
    * @param hub hub number of the device
    * @param port port number of the device
    * @param hub_parent reference to the hub where the device is connected (NULL if the hub parent is the root hub)
    * @param addr list of the TDs which have been completed to dequeue freed TDs
    */
    virtual void deviceDisconnected(int hub, int port, USBHostHub * hub_parent, volatile uint32_t addr) = 0;

    /**
    * Virtual method called when a transfer has been completed
    *
    * @param addr list of the TDs which have been completed
    */
    virtual void transferCompleted(volatile uint32_t addr) = 0;

    /**
    * Find a memory section for a new ED
    *
    * @returns the address of the new ED
    */
    volatile uint8_t * getED();

    /**
    * Find a memory section for a new TD
    *
    * @returns the address of the new TD
    */
    volatile uint8_t * getTD();

    /**
    * Release a previous memory section reserved for an ED
    *
    * @param ed address of the ED
    */
    void freeED(volatile uint8_t * ed);

    /**
    * Release a previous memory section reserved for an TD
    *
    * @param td address of the TD
    */
    void freeTD(volatile uint8_t * td);

#if defined(TARGET_UNO_91H)
    int control_transfer(uint8_t* data, uint16_t wLength);
    CONTROL_TRANSFER * getTransferPtr();
    virtual bool EP0Continue(){return false;};
    virtual bool EP0Status(){return false;};
    virtual void EP0Complete(){};
    void HostEndpointRead(uint8_t endpoint, uint8_t *buffer, uint32_t len);
    void HostEndpointWrite(uint8_t endpoint, uint8_t *buffer, uint32_t len);
    bool dma_channel_program(uint8_t ep_num,uint8_t transmit,uint8_t mode, uint32_t dma_addr, uint32_t len);
    virtual void OutEpCallback(uint8_t endpoint){};
    virtual void InEpCallback(uint8_t endpoint){};
#ifdef CFG_USB_DMA
    virtual void dmaCallback(uint8_t channel) {};
    virtual void usb_dma_completion(uint8_t channel){};
    virtual uint8_t get_dma_channel_status(uint8_t channel) { return 0; };
    virtual void set_dma_channel_status(uint8_t channel, uint8_t status){};
#endif
#endif /*TARGET_UNO_91H*/

private:
    static void _usbisr(void);
    void UsbIrqhandler();
#if defined(TARGET_UNO_91H)
    void usb_dma_isr(uint8_t hsdma);
#endif
    void memInit();

    HCCA volatile * usb_hcca;           //256 bytes aligned
    uint8_t volatile  * usb_edBuf;      //4 bytes aligned
    uint8_t volatile  * usb_tdBuf;      //4 bytes aligned

    static USBHALHost * instHost;

    bool volatile  edBufAlloc[MAX_ENDPOINT];
    bool volatile tdBufAlloc[MAX_TD];
};

#endif
