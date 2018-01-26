#include "mbed.h"
#include "USBHostMSD.h"
#include "FATFileSystem.h"
#include <stdlib.h>
#include "USBMsg.h"

int main(){
    unsigned int msg;
    void *main_msgQ;

    USBHostMSD msd("usb");
    main_msgQ = usb_msgQ_create(5);
    msd.setMsgQueue(main_msgQ);

    while (true) {
        usb_msg_get(main_msgQ, &msg, osWaitForever);
        switch (msg)
        {
            case USB_CONNECT:
                printf("device connect \r\n");
                msd.connect();
                /* read or write disk or other operations*/
               break;
            case USB_DISCONNECT:
                printf("device disconnect \r\n");
                break;
            default:
                break;
        }
    }
}
