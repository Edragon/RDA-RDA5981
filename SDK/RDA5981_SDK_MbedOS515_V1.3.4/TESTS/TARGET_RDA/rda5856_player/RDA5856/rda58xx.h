/** file rda58xx.h
 * Headers for interfacing with the codec chip.
 */

#ifndef RDA58XX_H
#define RDA58XX_H

#include "mbed.h"
#include "rda58xx_int_types.h"

typedef enum {
    MCI_TYPE_NONE=-1,
    MCI_TYPE_GSM_FR,              /* 0  */
    MCI_TYPE_GSM_HR,              /* 1  */
    MCI_TYPE_GSM_EFR,             /* 2  */
    MCI_TYPE_AMR,                 /* 3  */
    MCI_TYPE_AMR_WB,              /* 4  */
    MCI_TYPE_DAF,                 /* 5  */
    MCI_TYPE_AAC,                 /* 6  */
    MCI_TYPE_PCM_8K,              /* 7  */
    MCI_TYPE_PCM_16K,             /* 8  */
    MCI_TYPE_G711_ALAW,           /* 9  */
    MCI_TYPE_G711_ULAW,           /* 10 */
    MCI_TYPE_DVI_ADPCM,           /* 11 */
    MCI_TYPE_VR,                  /* 12 */
    MCI_TYPE_WAV,                 /* 13 */
    MCI_TYPE_WAV_ALAW,            /* 14 */
    MCI_TYPE_WAV_ULAW,            /* 15 */
    MCI_TYPE_WAV_DVI_ADPCM,       /* 16 */
    MCI_TYPE_SMF,                 /* 17 */
    MCI_TYPE_IMELODY,             /* 18 */
    MCI_TYPE_SMF_SND,             /* 19 */
    MCI_TYPE_MMF,                 /* 20 */
    MCI_TYPE_AU,                  /* 21 */
    MCI_TYPE_AIFF,                /* 22 */
    MCI_TYPE_M4A,                 /* 23 */
    MCI_TYPE_3GP,                 /* 24 */
    MCI_TYPE_MP4,                 /* 25 */
    MCI_TYPE_JPG,                 /* 26 */
    MCI_TYPE_GIF,                 /* 27 */
    MCI_TYPE_MJPG,                /* 28 */
    MCI_TYPE_WMA,                 /* 29 */
    MCI_TYPE_MIDI,                /* 30 */
    MCI_TYPE_RM,                  /* 31 */
    //MCI_TYPE_AVSTRM,              /* 32 */
    MCI_TYPE_SBC,                 /* 32 */
    MCI_TYPE_SCO,                 /* 33 */
    MCI_TYPE_TONE,                /* 34 */
    MCI_TYPE_USB,                 /* 35 */
    MCI_TYPE_LINEIN,              /* 36 */
    MCI_NO_OF_TYPE
} mci_type_enum;

typedef enum {
    UNREADY = -1,
    STOP = 0,
    PLAY,
    PAUSE,
    RECORDING,
    STOP_RECORDING,
    MODE_SWITCHING
} rda58xx_status;

typedef enum {
    EMPTY = 0,
    FULL
} rda58xx_buffer_status;

typedef enum {
    WITHOUT_ENDING = 0,
    WITH_ENDING
} rda58xx_stop_type;

typedef enum {
    NACK = 0, //No ACK
    IACK,     //Invalid ACK
    VACK      //Valid ACK
} rda58xx_at_status;

typedef enum {
    FT_DISABLE = 0,
    FT_ENABLE
} rda58xx_ft_test;

typedef enum {
    UART_MODE = 0,
    BT_MODE
} rda58xx_mode;

typedef struct {
    uint8_t *buffer;
    uint8_t bufferSize;
    rda58xx_status status;
    int value;
} rda58xx_parameter;

class rda58xx
{
public:
    rda58xx(PinName TX, PinName RX, PinName HRESET);
    ~rda58xx(void);
    void hardReset(void);
    rda58xx_at_status bufferReq(mci_type_enum ftype, uint16_t size, uint16_t threshold);
    rda58xx_at_status stopPlay(void);
    rda58xx_at_status stopPlay(rda58xx_stop_type stype);
    rda58xx_at_status pause(void);
    rda58xx_at_status resume(void);
    rda58xx_at_status startRecord(mci_type_enum ftype, uint16_t size, uint16_t sampleRate = 8000);
    rda58xx_at_status stopRecord(void);
    rda58xx_at_status setMicGain(uint8_t gain);
    rda58xx_at_status volAdd(void);
    rda58xx_at_status volSub(void);
    rda58xx_at_status volSet(uint8_t vol);
    rda58xx_at_status sendRawData(uint8_t *databuf, uint16_t n);
    void setBaudrate(int32_t baud);
    void setStatus(rda58xx_status status);
    rda58xx_status getStatus(void);
    void clearBufferStatus(void);
    rda58xx_buffer_status getBufferStatus(void);
    uint8_t *getBufferAddr(void);
    void setBufferSize(uint32_t size);
    uint32_t getBufferSize(void);
    rda58xx_at_status factoryTest(rda58xx_ft_test mode);
    rda58xx_at_status getCodecStatus(rda58xx_status *status);
    rda58xx_at_status getChipVersion(char *version);
    rda58xx_at_status getBtRssi(int8_t *RSSI);
    rda58xx_at_status setMode(rda58xx_mode mode);
    rda58xx_mode getMode(void);
    rda58xx_at_status setBtBrMode(void);
    rda58xx_at_status setBtLeMode(void);
    rda58xx_at_status lesWifiScs(void);
    rda58xx_at_status getApSsid(char *SSID);
    rda58xx_at_status getApPwd(char *PWD);
    bool isReady(void);
    bool isPowerOn(void);
    int32_t (*atHandler) (int32_t status);
    int32_t setAtHandler(int32_t (*handler)(int32_t status));
    void rx_handler(void);
    void tx_handler(void);
private:
    RawSerial  _serial;
    int32_t    _baud;
    Semaphore  _rxsem;
    DigitalOut _HRESET;
    volatile rda58xx_mode _mode;
    volatile rda58xx_status _status;
    volatile rda58xx_buffer_status _rx_buffer_status;
    uint32_t _rx_buffer_size;
    uint32_t _tx_buffer_size;
    uint32_t _rx_idx;
    uint32_t _tx_idx;
    uint8_t *_rx_buffer;
    uint8_t *_tx_buffer;
    rda58xx_parameter _parameter;
    volatile bool _with_parameter;
    volatile rda58xx_at_status _at_status;
    volatile bool _ready;
    volatile bool _power_on;
    uint32_t _ats;
    uint32_t _atr;
};

#endif

