#include "mbed.h"
#include "player.h"
#include "gpadckey.h"

GpadcKey KEY_PS(KEY_A0);
GpadcKey KEY_Next(KEY_A1);
GpadcKey KEY_Pre(KEY_A2);
GpadcKey KEY_Up(KEY_A3);
GpadcKey KEY_Down(KEY_A4);
GpadcKey KEY_Switch(KEY_A5);

Player player;
Timer timer;

extern char list[15][32];       //song list
extern uint8_t index_current;      //song play index_current
extern uint8_t index_MAX;          //how many song in all
//extern unsigned char vlume;     //vlume
//extern unsigned char vlumeflag; //set vlume flag
extern playerStatetype  playerState;
extern volumeStatetype volumeState;
bool switchMode = false;

uint32_t falltime;
uint32_t fallCount = 0;
uint32_t riseCount = 0;

void fallFlip()
{
    fallCount++;
    if ((riseCount + 1) != fallCount) {
        riseCount = fallCount - 1;
    }
    falltime=timer.read_ms();
    mbed_error_printf("%s\r\n", __func__);
}

void riseFlip()
{
    riseCount++;
    if (riseCount != fallCount) {
        riseCount = fallCount;
        return;
    }
    if((timer.read_ms() - falltime) > 1000) {   //1s
        playerState = PS_RECORDING;             //long press for recording
    } else {
        if (playerState == PS_PAUSE)
            //playerState = PS_PLAY;              //play or pause
            playerState = PS_RESUME;              //play or pause
        else
            playerState = PS_PAUSE;
    }
    mbed_error_printf("%s\r\n", __func__);
}

void Pre()
{
    index_current = (index_current - 1 < 0) ? index_MAX : index_current - 1;
    playerState = PS_STOP;
    mbed_error_printf("%s, index_current=%d\r\n", __func__, index_current);
}

void Next()
{
    index_current = (index_current + 1 > index_MAX) ? 0 : index_current + 1;
    playerState = PS_STOP;
    mbed_error_printf("%s, index_current=%d\r\n", __func__, index_current);
}    

void Up()
{
    volumeState = VOL_ADD;
    mbed_error_printf("%s\r\n", __func__);
}

void Down()
{
    volumeState = VOL_SUB;
    mbed_error_printf("%s\r\n", __func__);
}

void Switch()
{
    playerState = PS_STOP;
    switchMode = true;
    mbed_error_printf("%s\r\n", __func__);
}

int main()
{
    KEY_PS.fall(&fallFlip);
    KEY_PS.rise(&riseFlip);
    KEY_Pre.fall(&Pre);
    KEY_Next.fall(&Next);
    KEY_Up.fall(&Up);
    KEY_Down.fall(&Down);
    KEY_Switch.fall(&Switch);

    timer.start();
    player.begin();
    
    while(!player.isReady()) {
        printf("Codec Not Ready\n");
        Thread::wait(100);
    }
    printf("Codec Ready\n");

#if defined(BT_DEBUG)
    while (1) {
        while (UART_MODE == player.getMode()) {
#if 1

            player.playFile(list[index_current]);
            printf("AfterPlay\r\n");
            if (playerState == PS_RECORDING) {
                player.recordFile("/recording.WAV");
                player.playFile("/recording.WAV");
            }
#endif
            if (switchMode) {
                player.setBtMode();
                printf("BT MODE\n");
                switchMode = false;
            }
        }
        while (BT_MODE == player.getMode()) {
            Thread::wait(100);
            if (switchMode) {
                player.setUartMode();
                while (!player.isReady()) {
                    Thread::wait(100);
                }
                printf("UART MODE\n");
                switchMode = false;
            }
        }
    }
#else
    while(1) {
        player.playFile(list[index_current]);
        printf("AfterPlay\r\n");
        if (playerState == PS_RECORDING) {
            player.recordFile("/recording.WAV");
            player.playFile("/recording.WAV");
        }
    }
#endif
}

