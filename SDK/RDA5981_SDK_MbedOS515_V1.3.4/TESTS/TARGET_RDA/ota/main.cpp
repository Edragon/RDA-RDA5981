#include "mbed.h"
#include "rtos.h"
#include "WiFiStackInterface.h"

#include "rda5981_ota.h"

int main() {
    WiFiStackInterface wifi;
    uint8_t buf[4096];
    int i;
    int ret;

    printf("\r\n######Start ota test######\r\n");

    wifi.connect("Tenda_4593A8", "12345678", NULL, NSAPI_SECURITY_NONE);

    rda5981_write_partition_start(0x18080000, 4096*10);

    for (i=0; i<10; ++i) {
        memcpy(buf, (uint8_t *)(0x18010000 + i*4096), 4096);
        rda5981_write_partition(4096*i, (const uint8_t *)buf, 4096);
    }

    ret = rda5981_write_partition_end();

    printf("write partition end:%d\r\n", ret);

    while (true) {
    }
}

