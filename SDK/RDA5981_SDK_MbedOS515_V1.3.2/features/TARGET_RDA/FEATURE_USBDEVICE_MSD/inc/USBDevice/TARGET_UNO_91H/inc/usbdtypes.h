
#ifndef _USBDTYPES_H
#define _USBDTYPES_H

#ifndef BIT
#define BIT(x) (1 << (x))
#endif

#ifndef SETUP_PACKET_T
typedef struct {
    uint8_t  bmRequestType;
    uint8_t  bRequest;
    uint16_t wValue;
    uint16_t wIndex;
    uint16_t wLength;
} SETUP_PACKET_T;
#endif

#define USB_MIN_EP_NUM  1
#define USB_MAX_EP_NUM  4
#define USB_EP0_MAX_PACKET_SIZE 64
#define USB_NUM_INTERFACE   2

#endif
