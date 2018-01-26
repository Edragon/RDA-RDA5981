#ifndef USBMSG_H
#define USBMSG_H
#ifdef __cplusplus
extern "C" {
#endif
#define WaitForever                  0xFFFFFFFF
typedef enum
{
	USB_CONNECT = 0,
	USB_DISCONNECT
} USB_HOST_MSG;

/* Queue */
/**
 * @brief     : create a message queue
 * @param[in] : size of message queue
 * @param[out]:
 * @return    : return message queue id or NULL if error
 */
extern void* usb_msgQ_create(unsigned int queuesz);

/**
 * @brief     : put a message to queue
 * @param[in] : message queue id, message value and wait time
 * @param[out]:
 * @return    : return ERR or NO_ERR
 */
extern int usb_msg_put(void *msgQId, unsigned int msg, unsigned int millisec);

/**
 * @brief     : get a message from queue
 * @param[in] : message queue id, message value and wait time
 * @param[out]:
 * @return    : return ERR or NO_ERR
 */
extern int usb_msg_get(void *msgQId, unsigned int *value, unsigned int millisec);
#ifdef __cplusplus
}
#endif

#endif /* USBMSG_H */
