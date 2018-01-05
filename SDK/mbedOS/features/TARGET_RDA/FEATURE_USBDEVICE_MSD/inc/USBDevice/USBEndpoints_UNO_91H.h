#if defined(TARGET_UNO_91H)

#ifndef _USBENDPOINTS_UNO_91H_H
#define _USBENDPOINTS_UNO_91H_H

#define MAX_PACKET_SIZE_EP0 (64)
#define MAX_PACKET_SIZE_EP1  (512)
#define MAX_PACKET_SIZE_EP2 (512)
#define MAX_PACKET_SIZE_EP3_ISO (1023)

#define EP0OUT      (0)
#define EP0IN       (0)
#define EP1OUT      (1)
#define EP1IN       (2)
#define EP2OUT      (5)
#define EP2IN       (4)
#define EP3OUT      (6)
#define EP3IN       (7)
///TODO: endpoint bumber is not match with callback
/*
    BY liliu
    TX:
    ep_num callback
      1      1
      2      3
      n     ((n-1) << 1) + 1

    RX:
    ep_num callback
      1      0
      2      2
      3      4
      n     ((n-1) << 1)

    NOTE: the ep_num is the hardware enpoint number
    And the EP1OUT = 1, is  hardware endpoint 1
    EP1IN = 2, is hardware endpoint 2
    thus, the EPBULK_IN_callback is EP2_IN_callback.
*/

/* Bulk endpoints */
#define EPBULK_OUT  (EP1OUT)
#define EPBULK_IN   (EP1IN)
#define EPBULK_OUT_callback   EP1_OUT_callback
#define EPBULK_IN_callback    EP2_IN_callback

/* Isochronous endpoint */
#define EPISO_OUT   (EP3OUT)
#define EPISO_IN    (EP3IN)
#define EPISO_OUT_callback    EP1_OUT_callback
#define EPISO_IN_callback     EP3_IN_callback

/* Interrupt endpoint */
#define EPINT_OUT   (EP2OUT)
#define EPINT_IN    (EP2IN)
#define EPINT_OUT_callback    EP2_OUT_callback
#define EPINT_IN_callback     EP2_IN_callback

#define MAX_PACKET_SIZE_EPBULK  (MAX_PACKET_SIZE_EP1)
#define MAX_PACKET_SIZE_EPINT   (MAX_PACKET_SIZE_EP2)
#define MAX_PACKET_SIZE_EPISO   (MAX_PACKET_SIZE_EP3_ISO)

#define BIT0    (1UL<<0)
#define BIT1    (1UL<<1)
#define BIT2    (1UL<<2)
#define BIT3    (1UL<<3)
#define BIT4    (1UL<<4)
#define BIT5    (1UL<<5)
#define BIT6    (1UL<<6)
#define BIT7    (1UL<<7)

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

/* CSR0 */
#define USB_CSR0_FLUSHFIFO              (0x0100)
#define USB_CSR0_TXPKTRDY               (0x0002)
#define USB_CSR0_RXPKTRDY               (0x0001)

/* CSR0 in Peripheral mode */
#define USB_CSR0_P_SVDSETUPEND          (0x0080)
#define USB_CSR0_P_SVDRXPKTRDY          (0x0040)
#define USB_CSR0_P_SENDSTALL            (0x0020)
#define USB_CSR0_P_SETUPEND             (0x0010)
#define USB_CSR0_P_DATAEND              (0x0008)
#define USB_CSR0_P_SENTSTALL            (0x0004)

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

/* TXCSR in Peripheral mode */
#define USB_TXCSR_P_ISO                 (0x4000)
#define USB_TXCSR_P_INCOMPTX            (0x0080)
#define USB_TXCSR_P_SENTSTALL           (0x0020)
#define USB_TXCSR_P_SENDSTALL           (0x0010)
#define USB_TXCSR_P_UNDERRUN            (0x0004)

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

/* RXCSR in Peripheral mode */
#define USB_RXCSR_P_ISO                 (0x4000)
#define USB_RXCSR_P_SENTSTALL           (0x0040)
#define USB_RXCSR_P_SENDSTALL           (0x0020)
#define USB_RXCSR_P_OVERRUN             (0x0004)

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

#define USB_CONFIGDATA_MPRXE            (0x80)
#define USB_CONFIGDATA_MPTXE            (0x40)
#define USB_CONFIGDATA_BIGENDIAN        (0x20)
#define USB_CONFIGDATA_HBRXE            (0x10)
#define USB_CONFIGDATA_HBTXE            (0x08)
#define USB_CONFIGDATA_DYNFIFO          (0x04)
#define USB_CONFIGDATA_SOFTCONE         (0x02)
#define USB_CONFIGDATA_UTMIDW           (0x01)

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

typedef enum
{
    USB_EP0_STAGE_IDLE = 0,
    USB_EP0_STAGE_SETUP,
    USB_EP0_STAGE_TX,
    USB_EP0_STAGE_RX,
    USB_EP0_STAGE_STATUSIN,
    USB_EP0_STAGE_STATUSOUT,
    USB_EP0_STAGE_ACKWAIT
} USB_EP0_STAGE_T;
#endif
#endif
