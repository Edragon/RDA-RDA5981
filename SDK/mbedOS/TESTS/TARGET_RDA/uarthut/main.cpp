#include "mbed.h"
#include "rtos.h"
#include "WiFiStackInterface.h"
#include "rda_sys_wrapper.h"
#include "wland_rf.h"

extern void start_at(void);

int main()
{
    WiFiStackInterface wifi;

    printf("Start uarthut test...\r\n");
    
    start_at();
    wifi.connect(NULL, NULL, NULL, NSAPI_SECURITY_NONE);
    
    while (true);
}

