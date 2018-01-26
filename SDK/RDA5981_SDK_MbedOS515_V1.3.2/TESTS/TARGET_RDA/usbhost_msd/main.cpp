#include "mbed.h"
#include "USBHostMSD.h"
#include "FATFileSystem.h"
#include <stdlib.h>

#define BUF_LENGTH (32 * 1024)

extern unsigned int os_time;
char buf[BUF_LENGTH] = {0};

int main(){
    const char *file_name0 = "/usb/usbmsdtest.txt";
    int idx = 0;
    FILE *fp = NULL;
    long t1, rLen, totalLen= 0;
    int len = 32 * 1024;

    memset(buf, 0xff, BUF_LENGTH);

    USBHostMSD msd("usb");

    wait(5);
    msd.connect();

    printf("usb host msd test begin\r\n");

    fp = fopen(file_name0, "at+");

    if (NULL == fp) {
        error("Could not open file for binary write!\r\n");
        goto done;
    }
    printf("fwrite test begin\r\n");

    t1 = os_time;
    for (idx = 0; idx < 10000; idx++) {
        rLen = fwrite(buf, sizeof(char), len, fp);
        fflush(fp);

        if (rLen == 0)
            break;
        totalLen += rLen;
        if (os_time - t1 > 1000) {
            printf("f_write Speed %0.5f KB/s\r\n", totalLen * 1000.0 / 1024 / (os_time- t1));
            t1 = os_time;
            totalLen = 0;
        }
    }
    printf("fwrite test end\r\n");
    printf("Please wait 10 seconds before read test begin\r\n");
    fclose(fp);

    wait(10);
    fp = fopen(file_name0, "r+");
    if (NULL == fp) {
        error("Could not open file for binary write!\r\n");
        goto done;
    }
    printf("fread test begin\r\n");
    t1 = os_time;
    totalLen = 0;
    for (idx = 0; idx < 10000; idx++) {
        rLen = fread(buf, sizeof(char), len, fp);
        totalLen += rLen;

        if(os_time - t1 > 1000) {
            printf("f_read Speed %0.5f KB/s\r\n", totalLen * 1000.0 / 1024 / (os_time- t1));
            t1 = os_time;
            totalLen = 0;
        }
    }
    printf("fread test end\r\n");
    fclose(fp);
done:
    printf("usb host msd test end \r\n");
}
