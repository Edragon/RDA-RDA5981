#ifndef PLAYER_H
#define PLAYER_H

#include "rda58xx.h"

/** Playing states definations. */
volatile typedef enum playerStatetype_t {
    PS_STOP,         //Player stop
    PS_PLAY,         //Start to playe
    PS_PAUSE,        //Pause play
    PS_RESUME,       //Resume play
    PS_RECORDING,    //Recording states
} playerStatetype;

/** Control states definations. */
volatile typedef enum ctrlStatetype_t {
    CS_EMPTY = 0,      // Have no control
    CS_PLAYPAUSE,      // Play/pause button pressed
    CS_RECORDING,      // Play/pause button long pressed
    CS_UP,             // Up button pressed
    CS_DOWN,           // Down button pressed
    CS_NEXT,           // Right button pressed
    CS_PREV,           // Left button pressed
} ctrlStatetype;

volatile typedef enum volumeStatetype_t {
    VOL_NONE = 0,
    VOL_ADD,
    VOL_SUB,
} volumeStatetype;

class Player
{
public:
    void begin(void);
    void playFile(const char *file);
    int32_t commonPlay(FileHandle *fp);
    int32_t m4aPlay(FileHandle *fp);
    void recordFile(const char *file);
    bool isReady(void);
    void setUartMode(void);
    void setBtMode(void);
    rda58xx_mode getMode(void);
    int8_t getBtRssi(void);
private:

};

#endif
