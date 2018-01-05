#if defined(TARGET_UNO_91H)

#pragma once
#include "mbed.h"
#include "USBHostTypes.h"
#include "USBEndpoint.h"
#include "RDA5991H.h"

/* for 5981, endpoint fifosize as follows:
 * | ep num | fifosize |
 * |--------|----------|
 * |    1   |  0x93    |
 * |    2   |  0x39    |
 * |    3   |  0x66    |
 * |    4   |  0xf9    |
 * FIFOSIZE reg:
 * |  D7      D4  |  D3      D0  |
 * |--------------|--------------|
 * | RX FIFO Size | TX FIFO Size |
 * bulk enpoint fifo size is 512, so bulk out ep is 2, bulk in ep is 1.
 */
/* Bulk endpoints */
#define EPBULK_OUT  (2)
#define EPBULK_IN   (1)

#define BIT0    (1UL<<0)
#define BIT1    (1UL<<1)
#define BIT2    (1UL<<2)
#define BIT3    (1UL<<3)
#define BIT4    (1UL<<4)
#define BIT5    (1UL<<5)
#define BIT6    (1UL<<6)
#define BIT7    (1UL<<7)

#ifndef BIT
#define BIT(x) (1 << (x))
#endif

#define USB_MIN_EP_NUM  1
#define USB_MAX_EP_NUM  4
#define USB_EP0_MAX_PACKET_SIZE 64
#define USB_NUM_INTERFACE   2


#define USB_POWER_ISOUPDATE             (BIT7)
#define USB_POWER_SOFTCONN              (BIT6)
#define USB_POWER_HSENAB                (BIT5)
#define USB_POWER_HSMODE                (BIT4)
#define USB_POWER_RESET                 (BIT3)
#define USB_POWER_RESUME                (BIT2)
#define USB_POWER_SUSPENDM              (BIT1)
#define USB_POWER_ENSUSPEND             (BIT0)

#define USB_INTR_VBUSERROR              (BIT7)
#define USB_INTR_SESSREQ                (BIT6)
#define USB_INTR_DISCONNECT             (BIT5)
#define USB_INTR_CONNECT                (BIT4)
#define USB_INTR_SOF                    (BIT3)
#define USB_INTR_BABBLE                 (BIT2)
#define USB_INTR_RESET                  (BIT2)
#define USB_INTR_RESUME                 (BIT1)
#define USB_INTR_SUSPEND                (BIT0)

#define USB_TEST_FORCE_HOST             (0x80)
#define USB_TEST_FIFO_ACCESS            (0x40)
#define USB_TEST_FORCE_FS               (0x20)
#define USB_TEST_FORCE_HS               (0x10)
#define USB_TEST_PACKET                 (0x08)
#define USB_TEST_K                      (0x04)
#define USB_TEST_J                      (0x02)
#define USB_TEST_SE0_NAK                (0x01)

/* DEVCTL */
#define USB_DEVCTL_BDEVICE              (0x80)
#define USB_DEVCTL_FSDEV                (0x40)
#define USB_DEVCTL_LSDEV                (0x20)
#define USB_DEVCTL_VBUS                 (0x18)
#define USB_DEVCTL_VBUS_SHIFT           (3)
#define USB_DEVCTL_HM                   (0x04)
#define USB_DEVCTL_HR                   (0x02)
#define USB_DEVCTL_SESSION              (0x01)

/* CSR0 */
#define USB_CSR0_FLUSHFIFO              (0x0100)
#define USB_CSR0_TXPKTRDY               (0x0002)
#define USB_CSR0_RXPKTRDY               (0x0001)

/* CSR0 in Host mode */
#define USB_CSR0_H_DIS_PING             (0x0800)
#define USB_CSR0_H_WR_DATATOGGLE        (0x0400)  /* Set to allow setting: */
#define USB_CSR0_H_DATATOGGLE           (0x0200)  /* Data toggle control */
#define USB_CSR0_H_NAKTIMEOUT           (0x0080)
#define USB_CSR0_H_STATUSPKT            (0x0040)
#define USB_CSR0_H_REQPKT               (0x0020)
#define USB_CSR0_H_ERROR                (0x0010)
#define USB_CSR0_H_SETUPPKT             (0x0008)
#define USB_CSR0_H_RXSTALL              (0x0004)

/* TXCSR in Peripheral and Host mode */
#define USB_TXCSR_AUTOSET               (0x8000)
#define USB_TXCSR_MODE                  (0x2000)
#define USB_TXCSR_DMAENAB               (0x1000)
#define USB_TXCSR_FRCDATATOG            (0x0800)
#define USB_TXCSR_DMAMODE               (0x0400)
#define USB_TXCSR_CLRDATATOG            (0x0040)
#define USB_TXCSR_FLUSHFIFO             (0x0008)
#define USB_TXCSR_FIFONOTEMPTY          (0x0002)
#define USB_TXCSR_TXPKTRDY              (0x0001)

/* TXCSR in Host mode */
#define USB_TXCSR_H_WR_DATATOGGLE       (0x0200)
#define USB_TXCSR_H_DATATOGGLE          (0x0100)
#define USB_TXCSR_H_NAKTIMEOUT          (0x0080)
#define USB_TXCSR_H_RXSTALL             (0x0020)
#define USB_TXCSR_H_ERROR               (0x0004)

/* TXCSR bits to avoid zeroing (write zero clears, write 1 ignored) */
#define USB_TXCSR_P_WZC_BITS   \
    (USB_TXCSR_P_INCOMPTX | USB_TXCSR_P_SENTSTALL \
    | USB_TXCSR_P_UNDERRUN | USB_TXCSR_FIFONOTEMPTY)

#define USB_TXCSR_H_WZC_BITS   \
    (USB_TXCSR_H_NAKTIMEOUT | USB_TXCSR_H_RXSTALL \
    | USB_TXCSR_H_ERROR | USB_TXCSR_FIFONOTEMPTY)

/* RXCSR in Peripheral and Host mode */
#define USB_RXCSR_AUTOCLEAR             (0x8000)
#define USB_RXCSR_DMAENAB               (0x2000)
#define USB_RXCSR_DISNYET               (0x1000)
#define USB_RXCSR_PID_ERR               (0x1000)
#define USB_RXCSR_DMAMODE               (0x0800)
#define USB_RXCSR_INCOMPRX              (0x0100)
#define USB_RXCSR_CLRDATATOG            (0x0080)
#define USB_RXCSR_FLUSHFIFO             (0x0010)
#define USB_RXCSR_DATAERROR             (0x0008)
#define USB_RXCSR_FIFOFULL              (0x0002)
#define USB_RXCSR_RXPKTRDY              (0x0001)

/* RXCSR in Host mode */
#define USB_RXCSR_H_AUTOREQ             (0x4000)
#define USB_RXCSR_H_WR_DATATOGGLE       (0x0400)
#define USB_RXCSR_H_DATATOGGLE          (0x0200)
#define USB_RXCSR_H_RXSTALL             (0x0040)
#define USB_RXCSR_H_REQPKT              (0x0020)
#define USB_RXCSR_H_ERROR               (0x0004)

/* RXCSR bits to avoid zeroing (write zero clears, write 1 ignored) */
#define USB_RXCSR_P_WZC_BITS    \
    (USB_RXCSR_P_SENTSTALL | USB_RXCSR_P_OVERRUN \
    | USB_RXCSR_RXPKTRDY)

#define USB_RXCSR_H_WZC_BITS    \
    (USB_RXCSR_H_RXSTALL | USB_RXCSR_H_ERROR \
    | USB_RXCSR_DATAERROR | USB_RXCSR_RXPKTRDY)
#define USB_RXCSR_H_ERR_BITS \
    (USB_RXCSR_H_RXSTALL | USB_RXCSR_H_ERROR \
    | USB_RXCSR_DATAERROR | USB_RXCSR_INCOMPRX)

#define USB_CONFIGDATA_MPRXE            (0x80)
#define USB_CONFIGDATA_MPTXE            (0x40)
#define USB_CONFIGDATA_BIGENDIAN        (0x20)
#define USB_CONFIGDATA_HBRXE            (0x10)
#define USB_CONFIGDATA_HBTXE            (0x08)
#define USB_CONFIGDATA_DYNFIFO          (0x04)
#define USB_CONFIGDATA_SOFTCONE         (0x02)
#define USB_CONFIGDATA_UTMIDW           (0x01)

#define USB_ANAREG2_OTGAVFILTER         (0x7000)
#define USB_ANAREG2_EXCHECKENABLE       (0x200000)

typedef enum
{
    USB_SPEED_UNKNOWN = 0,
    USB_SPEED_LOW,
    USB_SPEED_FULL,
    USB_SPEED_HIGH
} USB_SPEED_T;

#define DEV_ADDR_MASK   (0x7f)
#define DEV_ADDR(a)     ((a) & DEV_ADDR_MASK)

#define EP(endpoint) (1UL<<endpoint)

/* host side ep0 states */
typedef enum
{
    USB_EP0_IDLE = 0,
    USB_EP0_START,
    USB_EP0_IN,
    USB_EP0_OUT,
    USB_EP0_STATUS
} USB_EP0_STAGE_H;

/* control register (16-bit): */
#define USB_HSDMA_ENABLE_SHIFT        0
#define USB_HSDMA_TRANSMIT_SHIFT      1
#define USB_HSDMA_MODE1_SHIFT         2
#define USB_HSDMA_IRQENABLE_SHIFT     3
#define USB_HSDMA_ENDPOINT_SHIFT      4
#define USB_HSDMA_BUSERROR_SHIFT      8
#define USB_HSDMA_BURSTMODE_SHIFT     9
#define USB_HSDMA_BURSTMODE           (3 << USB_HSDMA_BURSTMODE_SHIFT)
#define USB_HSDMA_BURSTMODE_UNSPEC    0
#define USB_HSDMA_BURSTMODE_INCR4     1
#define USB_HSDMA_BURSTMODE_INCR8     2
#define USB_HSDMA_BURSTMODE_INCR16    3

/* MAX support 8 channel */
/* only 2 in 5991H */
#define USB_DMA_CHANNELS   (2)

typedef enum
{
    /* unallocated */
    USB_DMA_STATUS_UNKNOWN,
    /* allocated ... but not busy, no errors */
    USB_DMA_STATUS_FREE,
    /* busy ... transactions are active */
    USB_DMA_STATUS_BUSY,
    /* transaction(s) aborted due to ... dma or memory bus error */
    USB_DMA_STATUS_BUS_ABORT,
    /* transaction(s) aborted due to ... core error or USB fault */
    USB_DMA_STATUS_CORE_ABORT
} USB_DMA_STATUS;

#endif /* TARGET_UNO_91H */

