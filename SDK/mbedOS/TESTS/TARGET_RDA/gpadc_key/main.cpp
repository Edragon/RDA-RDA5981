#include "mbed.h"
#include "rtos.h"
#include "gpadckey.h"

GpadcKey ps_key(KEY_A0);
GpadcKey next_key(KEY_A1);
GpadcKey pre_key(KEY_A2);
GpadcKey up_key(KEY_A3);
GpadcKey down_key(KEY_A4);
GpadcKey smrtcfg_key(KEY_A5);

//Ticker t;
Timer timer;
int falltime;

void FallFlip()
{
    falltime = timer.read_ms();
}

void RiseFlip()
{
    int risetime = timer.read_ms();
    if((risetime - falltime) > 1000) {   //1s
        //long press for recording
        mbed_error_printf("%d-%d, Recording\r\n", falltime, risetime);
    } else {
        //play or pause
        mbed_error_printf("Play or Pause\r\n");
    }
}

void ChooseNext()
{
    mbed_error_printf("Next\r\n");
}

void ChoosePre()
{
    mbed_error_printf("Pre\r\n");
}

void VolumeUp()
{
    mbed_error_printf("Up\r\n");
}

void VolumeDown()
{
    mbed_error_printf("Down\r\n");
}

void SmartConfig()
{
    mbed_error_printf("Smart Config\r\n");
}

int main()
{
    printf("Start gpadc test...\r\n");

    ps_key.fall(&FallFlip);
    ps_key.rise(&RiseFlip);
    next_key.fall(&ChooseNext);
    pre_key.fall(&ChoosePre);
    up_key.fall(&VolumeUp);
    down_key.fall(&VolumeDown);
    smrtcfg_key.fall(&SmartConfig);

    timer.start();

    while(1) {
    }
}

