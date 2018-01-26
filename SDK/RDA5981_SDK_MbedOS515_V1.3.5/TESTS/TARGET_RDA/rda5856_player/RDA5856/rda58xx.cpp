/** \file rda58xx.c
 * Functions for interfacing with the codec chip.
 */
#include "rda58xx.h"
#include "rda58xx_dbg.h"
#include "rda58xx_int_types.h"
#include "mbed_interface.h"

#define CHAR_CR             (0x0D)  // carriage return
#define CHAR_LF             (0x0A)  // line feed

#define MUPLAY_PROMPT       "AT+MUPLAY="
#define TX_DATA_PROMPT      "AT+MURAWDATA="
#define RUSTART_PROMPT      "AT+RUSTART="

#define MUSTOP_CMD          "AT+MUSTOP="
#define MUSTOP_CMD2         "AT+MUSTOP\r\n"
#define RUSTOP_CMD          "AT+RUSTOP\r\n"
#define VOLADD_CMD          "AT+CVOLADD\r\n"
#define VOLSUB_CMD          "AT+CVOLSUB\r\n"
#define SET_VOL_CMD         "AT+CVOLUME="
#define MUPAUSE_CMD         "AT+MUPAUSE\r\n"
#define MURESUME_CMD        "AT+MURESUME\r\n"
#define SET_MIC_GAIN_CMD    "AT+MUSETMICGAIN="
#define FT_CMD              "AT+FTCODEC="
#define GET_STATUS_CMD      "AT+MUGETSTATUS\r\n"
#define GET_VERSION_CMD     "AT+CVERSION\r\n"
#define UART_MODE_CMD       "AT+CMODE=UART_PLAY\r\n"
#define BT_MODE_CMD         "AT+CMODE=BT\r\n"
#define BT_BR_MODE_CMD      "AT+BTSWITCH=0\r\n"
#define BT_LE_MODE_CMD      "AT+BTSWITCH=1\r\n"
#define GET_BT_RSSI_CMD     "AT+BTGETRSSI\r\n"
#define LESWIFISCS_CMD      "AT+LESWIFISCS\r\n"
#define GET_AP_SSID_CMD     "AT+LEGETSSID\r\n"
#define GET_AP_PWD_CMD      "AT+LEGETPASSWD\r\n"
#define RX_VALID_RSP        "\r\nOK\r\n"
#define RX_VALID_RSP2       "OK\r\n"
#define AT_READY_RSP        "AT Ready\r\n"
#define AST_POWERON_RSP     "AST_POWERON\r\n"
#define CODEC_READY_RSP     "CODEC_READY\r\n"

#define TIMEOUT_MS 5000

#define RDA58XX_DEBUG

uint8_t tmpBuffer[64] = {0};
uint8_t tmpIdx = 0;
uint8_t res_str[32] = {0};

#define r_GPIO_DOUT (*((volatile uint32_t *)0x40001008))
#define USE_TX_INT
int32_t atAckHandler(int32_t status);

/** Constructor of class RDA58XX. */
rda58xx::rda58xx(PinName TX, PinName RX, PinName HRESET):
    _serial(TX, RX),
    _baud(3200000),
    _rxsem(0),
    _HRESET(HRESET),
    _mode(UART_MODE),
    _status(UNREADY),
    _rx_buffer_status(EMPTY),
    _rx_buffer_size(640),
    _tx_buffer_size(0),
    _rx_idx(0),
    _tx_idx(0),
    _rx_buffer(NULL),
    _tx_buffer(NULL),
    _with_parameter(false),
    _at_status(NACK),
    _ready(false),
    _power_on(false),
    _ats(0),
    _atr(0)
{
    _serial.baud(_baud);
    _serial.attach(this, &rda58xx::rx_handler, RawSerial::RxIrq);
#if defined(USE_TX_INT)
    _serial.attach(this, &rda58xx::tx_handler, RawSerial::TxIrq);
    RDA_UART1->IER = RDA_UART1->IER & (~(1 << 1));//disable UART1 TX interrupt
    //RDA_UART1->FCR = 0x31;//set UART1 TX empty trigger FIFO 1/2 Full(0x21 1/4 FIFO)
#endif
    _HRESET = 1;
    _rx_buffer = new uint8_t [_rx_buffer_size * 2];
    _parameter.buffer = new uint8_t[64];
    _parameter.bufferSize = 0;
    atHandler = atAckHandler;
}

rda58xx::~rda58xx()
{
    if (_rx_buffer != NULL)
        delete [] _rx_buffer;
    if (_parameter.buffer != NULL)
        delete [] _parameter.buffer;
}

void rda58xx::hardReset(void)
{
    _HRESET = 0;
    wait_ms(1);
    _mode = UART_MODE;
    _status = UNREADY;
    _rx_buffer_status = EMPTY;
    _tx_buffer_size = 0;
    _rx_idx = 0;
    _tx_idx = 0;
    _with_parameter = false;
    _at_status = NACK;
    _ready = false;
    _power_on = false;
    _ats=0;
    _atr=0;
    _HRESET = 1;
}

rda58xx_at_status rda58xx::bufferReq(mci_type_enum ftype, uint16_t size, uint16_t threshold)
{
    uint8_t cmd_str[32] = {0};
    uint32_t len;

    // Send AT CMD: MUPLAY
    len = sprintf((char *)cmd_str, "%s%d,%d,%d\r\n", MUPLAY_PROMPT, ftype, size, threshold);
    cmd_str[len] = '\0';
    CODEC_LOG(INFO, "%s", cmd_str);
    _at_status = NACK;
    _with_parameter = false;
    _rx_idx = 0;
    core_util_critical_section_enter();
    _serial.puts((const char *)cmd_str);
    _ats++;
    core_util_critical_section_exit();
    _rxsem.wait(TIMEOUT_MS);
    _atr = _ats;

    return _at_status;
}

rda58xx_at_status rda58xx::stopPlay(void)
{
    // Send AT CMD: MUSTOP
    CODEC_LOG(INFO, MUSTOP_CMD);
    _at_status = NACK;
    _with_parameter = false;
    _rx_idx = 0;
    core_util_critical_section_enter();
    _serial.puts((const char *)MUSTOP_CMD2);
    _ats++;
    core_util_critical_section_exit();
    _rxsem.wait(TIMEOUT_MS);
    _atr = _ats;

    return _at_status;
}

rda58xx_at_status rda58xx::stopPlay(rda58xx_stop_type stype)
{
    uint8_t cmd_str[32] = {0};
    uint32_t len;

    // Send AT CMD: MUSTOP
    len = sprintf((char *)cmd_str, "%s%d\r\n", MUSTOP_CMD, stype);
    cmd_str[len] = '\0';
    CODEC_LOG(INFO, "%s", cmd_str);
    _at_status = NACK;
    _with_parameter = false;
    _rx_idx = 0;
    core_util_critical_section_enter();
    _serial.puts((const char *)cmd_str);
    _ats++;
    core_util_critical_section_exit();
    _rxsem.wait(TIMEOUT_MS);
    _atr = _ats;
    
    return _at_status;
}

rda58xx_at_status rda58xx::pause(void)
{
    // Send AT CMD: MUPAUSE
    CODEC_LOG(INFO, MUPAUSE_CMD);
    _at_status = NACK;
    _with_parameter = false;
    _rx_idx = 0;
    core_util_critical_section_enter();
    _serial.puts((const char *)MUPAUSE_CMD);
    _ats++;
    core_util_critical_section_exit();
    _rxsem.wait(TIMEOUT_MS);
    _atr = _ats;
    
    return _at_status;
}

rda58xx_at_status rda58xx::resume(void)
{
    // Send AT CMD: MURESUME
    CODEC_LOG(INFO, MURESUME_CMD);
    _at_status = NACK;
    _with_parameter = false;
    _rx_idx = 0;
    core_util_critical_section_enter();
    _serial.puts((const char *)MURESUME_CMD);
    _ats++;
    core_util_critical_section_exit();
    _rxsem.wait(TIMEOUT_MS);
    _atr = _ats;

    return _at_status;
}

rda58xx_at_status rda58xx::startRecord(mci_type_enum ftype, uint16_t size, uint16_t sampleRate)
{
    uint8_t cmd_str[32] = {0};
    uint32_t len;

    // Send AT CMD: RUSTART
    len = sprintf((char *)cmd_str, "%s%d,%d,%d\r\n", RUSTART_PROMPT, ftype, size, sampleRate);
    cmd_str[len] = '\0';
    CODEC_LOG(INFO, "%s", cmd_str);
    _at_status = NACK;
    _with_parameter = false;
    _rx_idx = 0;
    core_util_critical_section_enter();
    _serial.puts((const char *)cmd_str);
    _ats++;
    core_util_critical_section_exit();
    _rxsem.wait(TIMEOUT_MS);
    _atr = _ats;

    return _at_status;
}

rda58xx_at_status rda58xx::stopRecord(void)
{
    // Send AT CMD: RUSTOP
    CODEC_LOG(INFO, RUSTOP_CMD);
    _at_status = NACK;
    _status = STOP_RECORDING;
    _with_parameter = false;
    _rx_idx = 0;
    core_util_critical_section_enter();
    _serial.puts((const char *)RUSTOP_CMD);
    _ats++;
    core_util_critical_section_exit();
    _rxsem.wait(TIMEOUT_MS);
    _atr = _ats;

    return _at_status;
}

rda58xx_at_status rda58xx::setMicGain(uint8_t gain)
{
    uint8_t cmd_str[32] = {0};
    uint32_t len;

    // Send AT CMD: SET_MIC_GAIN
    gain = (gain < 12) ? gain : 12;
    len = sprintf((char *)cmd_str, "%s%d\r\n", SET_MIC_GAIN_CMD, gain);
    cmd_str[len] = '\0';
    CODEC_LOG(INFO, "%s", cmd_str);
    _at_status = NACK;
    _with_parameter = false;
    _rx_idx = 0;
    core_util_critical_section_enter();
    _serial.puts((const char *)cmd_str);
    _ats++;
    core_util_critical_section_exit();
    _rxsem.wait(TIMEOUT_MS);
    _atr = _ats;

    return _at_status;
}

rda58xx_at_status rda58xx::volAdd(void)
{
    // Send AT CMD: VOLADD
    CODEC_LOG(INFO, VOLADD_CMD);
    _at_status = NACK;
    _with_parameter = false;
    _rx_idx = 0;
    core_util_critical_section_enter();
    _serial.puts((const char *)VOLADD_CMD);
    _ats++;
    core_util_critical_section_exit();
    _rxsem.wait(TIMEOUT_MS);
    _atr = _ats;

    return _at_status;
}

rda58xx_at_status rda58xx::volSub(void)
{
    // Send AT CMD: VOLSUB
    CODEC_LOG(INFO, VOLSUB_CMD);
    _at_status = NACK;
    _with_parameter = false;
    _rx_idx = 0;
    core_util_critical_section_enter();
    _serial.puts((const char *)VOLSUB_CMD);
    _ats++;
    core_util_critical_section_exit();
    _rxsem.wait(TIMEOUT_MS);
    _atr = _ats;

    return _at_status;
}

rda58xx_at_status rda58xx::volSet(uint8_t vol)
{
    uint8_t cmd_str[32] = {0};
    uint32_t len;

    // Send AT CMD: SET_VOL
    vol = (vol < 15) ? vol : 15;
    len = sprintf((char *)cmd_str, "%s%d\r\n", SET_VOL_CMD, vol);
    cmd_str[len] = '\0';
    CODEC_LOG(INFO, "%s", cmd_str);
    _at_status = NACK;
    _with_parameter = false;
    _rx_idx = 0;
    core_util_critical_section_enter();
    _serial.puts((const char *)cmd_str);
    _ats++;
    core_util_critical_section_exit();
    _rxsem.wait(TIMEOUT_MS);
    _atr = _ats;

    return _at_status;
}

/** Send raw data to RDA58XX  */
rda58xx_at_status rda58xx::sendRawData(uint8_t *databuf, uint16_t n)
{
    uint8_t cmd_str[32] = {0};
    uint32_t len;

    if ((NULL == databuf) || (0 == n))
        return VACK;

    // Send AT CMD: MURAWDATA
    len = sprintf((char *)cmd_str, "%s%d\r\n", TX_DATA_PROMPT, n);
    cmd_str[len] = '\0';
    CODEC_LOG(INFO, "%s", cmd_str);
    _at_status = NACK;
    _with_parameter = false;
    _rx_idx = 0;
    core_util_critical_section_enter();
    _serial.puts((const char *)cmd_str);
    _ats++;
    core_util_critical_section_exit();
    _rxsem.wait(TIMEOUT_MS);
    _atr = _ats;
    if (VACK != _at_status) {
        CODEC_LOG(ERROR, "AT ACK=%d\n", _at_status);
        return _at_status;
    }

    // Raw Data bytes
    CODEC_LOG(INFO, "SendData\r\n");
    _at_status = NACK;
    _with_parameter = false;
    _rx_idx = 0;

#if defined(USE_TX_INT)
    core_util_critical_section_enter();
    _tx_buffer = databuf;
    _tx_buffer_size = n;
    _tx_idx = 0;
    RDA_UART1->IER = RDA_UART1->IER | (1 << 1);//enable UART1 TX interrupt
#else
    while(n--) {
        _serial.putc(*databuf);
        databuf++;
    }
#endif
    _ats++;
    core_util_critical_section_exit();
    _rxsem.wait(TIMEOUT_MS);
    _atr = _ats;

    return _at_status;
}

void rda58xx::setBaudrate(int32_t baud)
{
    _serial.baud(baud);
}

void rda58xx::setStatus(rda58xx_status status)
{
    _status = status;
}

rda58xx_status rda58xx::getStatus(void)
{
    return _status;
}

rda58xx_buffer_status rda58xx::getBufferStatus(void)
{
    return _rx_buffer_status;
}

void rda58xx::clearBufferStatus(void)
{
    _rx_buffer_status = EMPTY;
}

uint8_t *rda58xx::getBufferAddr(void)
{
    return &_rx_buffer[_rx_buffer_size];
}

/* Set the RX Buffer size.
 * You CAN ONLY use it when you are sure there is NO transmission on RX!!!
 */
void rda58xx::setBufferSize(uint32_t size)
{
    _rx_buffer_size = size;
    delete [] _rx_buffer;
    _rx_buffer = new uint8_t [_rx_buffer_size * 2];
}

uint32_t rda58xx::getBufferSize(void)
{
    return _rx_buffer_size;
}

rda58xx_at_status rda58xx::factoryTest(rda58xx_ft_test mode)
{
    uint8_t cmd_str[32] = {0};
    uint32_t len;

    // Send AT CMD: FT
    len = sprintf((char *)cmd_str, "%s%d\r\n", FT_CMD, mode);
    cmd_str[len] = '\0';
    CODEC_LOG(INFO, "%s", cmd_str);
    _at_status = NACK;
    _with_parameter = false;
    _rx_idx = 0;
    core_util_critical_section_enter();
    _serial.puts((const char *)cmd_str);
    _ats++;
    core_util_critical_section_exit();
    _rxsem.wait(TIMEOUT_MS);
    _atr = _ats;

    return _at_status;
}

rda58xx_at_status rda58xx::getCodecStatus(rda58xx_status *status)
{
    // Send AT CMD: GET_STATUS
    CODEC_LOG(INFO, GET_STATUS_CMD);
    _at_status = NACK;
    _with_parameter = true;
    _rx_idx = 0;
    core_util_critical_section_enter();
    _serial.puts((const char *)GET_STATUS_CMD);
    _ats++;
    core_util_critical_section_exit();
    _rxsem.wait(TIMEOUT_MS);
    _atr = _ats;

    if (VACK == _at_status) {
        if ((_parameter.buffer[0] >= '0') && (_parameter.buffer[0] <= '2')) {
            _parameter.status = (rda58xx_status)(_parameter.buffer[0] - 0x30);
        } else {
            _parameter.status = UNREADY;
        }
        CODEC_LOG(INFO, "status:%d\n", _parameter.status);
        _parameter.bufferSize = 0;
        *status = _parameter.status;
    }
    
    return _at_status;
}

rda58xx_at_status rda58xx::getChipVersion(char *version)
{
    CODEC_LOG(INFO, GET_VERSION_CMD);
    _at_status = NACK;
    _with_parameter = true;
    _rx_idx = 0;
    core_util_critical_section_enter();
    _serial.puts((const char *)GET_VERSION_CMD);
    _ats++;
    core_util_critical_section_exit();
    _rxsem.wait(TIMEOUT_MS);
    _atr = _ats;

    if (VACK == _at_status) {
        _parameter.buffer[_parameter.bufferSize] = '\0';
        memcpy(&version[0], &_parameter.buffer[0], (_parameter.bufferSize + 1));
        _parameter.bufferSize = 0;
    }
    
    return _at_status;
}

rda58xx_at_status rda58xx::getBtRssi(int8_t *RSSI)
{
    // Send AT CMD: GET_BT_RSSI
    CODEC_LOG(INFO, GET_BT_RSSI_CMD);
    _at_status = NACK;
    _with_parameter = true;
    _rx_idx = 0;
    core_util_critical_section_enter();
    _serial.puts((const char *)GET_BT_RSSI_CMD);
    _ats++;
    core_util_critical_section_exit();
    _rxsem.wait(TIMEOUT_MS);
    _atr = _ats;

    if (VACK == _at_status) {
        _parameter.value = 0;
        for (uint32_t i =0; i < _parameter.bufferSize; i++) {
            if ((_parameter.buffer[i] >= '0') && (_parameter.buffer[i] <= '9')) {
                _parameter.value *= 10;
                _parameter.value += _parameter.buffer[i] - '0';
            }
        }
        _parameter.bufferSize = 0;
        *RSSI = (int8_t)(_parameter.value & 0xFF);  
        CODEC_LOG(INFO, "BTRSSI:%d\n", *RSSI);
    }

    return _at_status;
}

/* Set mode, BT mode or Codec mode. In case of switching BT mode to Codec mode,
   set _status to MODE_SWITCHING and wait for "UART_READY" after receiving "OK" */
rda58xx_at_status rda58xx::setMode(rda58xx_mode mode)
{
    _at_status = NACK;
    _with_parameter = false;
    _rx_idx = 0;
    if (UART_MODE== mode) {
        // Send AT CMD: UART_MODE
        CODEC_LOG(INFO, UART_MODE_CMD);
        _status = MODE_SWITCHING;
        _ready = false;
        core_util_critical_section_enter();
        _serial.puts((const char *)UART_MODE_CMD);
        _ats++;
        core_util_critical_section_exit();
    } else if (BT_MODE== mode) {
        // Send AT CMD: BT_MODE
        CODEC_LOG(INFO, BT_MODE_CMD);
        core_util_critical_section_enter();
        _serial.puts((const char *)BT_MODE_CMD);
        _ats++;
        core_util_critical_section_exit();
    }
    _rxsem.wait(TIMEOUT_MS);
    _atr = _ats;
    if (VACK == _at_status) {
        _mode = mode;
        if (BT_MODE == mode)
            _ready = true;
    }

    return _at_status;
}

rda58xx_mode rda58xx::getMode(void)
{
    return _mode;
}

rda58xx_at_status rda58xx::setBtBrMode(void)
{
    // Send AT CMD: BT_BR_MODE
    CODEC_LOG(INFO, BT_BR_MODE_CMD);
    _at_status = NACK;
    _with_parameter = false;
    _rx_idx = 0;
    core_util_critical_section_enter();
    _serial.puts((const char *)BT_BR_MODE_CMD);
    _ats++;
    core_util_critical_section_exit();
    _rxsem.wait(TIMEOUT_MS);
    _atr = _ats;

    return _at_status;
}

rda58xx_at_status rda58xx::setBtLeMode(void)
{
    // Send AT CMD: BT_LE_MODE
    CODEC_LOG(INFO, BT_LE_MODE_CMD);
    _at_status = NACK;
    _with_parameter = false;
    _rx_idx = 0;
    core_util_critical_section_enter();
    _serial.puts((const char *)BT_LE_MODE_CMD);
    _ats++;
    core_util_critical_section_exit();
    _rxsem.wait(TIMEOUT_MS);
    _atr = _ats;

    return _at_status;
}

rda58xx_at_status rda58xx::lesWifiScs(void)
{
    // Send AT CMD: LESWIFISCS
    CODEC_LOG(INFO, LESWIFISCS_CMD);
    _at_status = NACK;
    _with_parameter = false;
    _rx_idx = 0;
    core_util_critical_section_enter();
    _serial.puts((const char *)LESWIFISCS_CMD);
    _ats++;
    core_util_critical_section_exit();
    _rxsem.wait(TIMEOUT_MS);
    _atr = _ats;

    return _at_status;
}

rda58xx_at_status rda58xx::getApSsid(char *SSID)
{
    // Send AT CMD: GET_AP_SSID
    CODEC_LOG(INFO, GET_AP_SSID_CMD);
    _at_status = NACK;
    _with_parameter = true;
    _rx_idx = 0;
    core_util_critical_section_enter();
    _serial.puts((const char *)GET_AP_SSID_CMD);
    _ats++;
    core_util_critical_section_exit();
    _rxsem.wait(TIMEOUT_MS);
    _atr = _ats;

    if (VACK == _at_status) {
        _parameter.buffer[_parameter.bufferSize] = '\0';
        memcpy(&SSID[0], &_parameter.buffer[0], (_parameter.bufferSize + 1));
        _parameter.bufferSize = 0;
    }

    return _at_status;
}

rda58xx_at_status rda58xx::getApPwd(char *PWD)
{
    // Send AT CMD: GET_AP_PWD
    CODEC_LOG(INFO, GET_AP_PWD_CMD);
    _at_status = NACK;
    _with_parameter = true;
    _rx_idx = 0;
    core_util_critical_section_enter();
    _serial.puts((const char *)GET_AP_PWD_CMD);
    _ats++;
    core_util_critical_section_exit();
    _rxsem.wait(TIMEOUT_MS);
    _atr = _ats;

    if (VACK == _at_status) {
        _parameter.buffer[_parameter.bufferSize] = '\0';
        memcpy(&PWD[0], &_parameter.buffer[0], (_parameter.bufferSize + 1));
        _parameter.bufferSize = 0;
    }
    
    return _at_status;
}


bool rda58xx::isReady(void)
{
    return _ready;
}

bool rda58xx::isPowerOn(void)
{
    return _power_on;
}

int32_t atAckHandler(int32_t status)
{
    if (VACK != (rda58xx_at_status) status) {
        switch ((rda58xx_at_status) status) {
        case NACK:
            CODEC_LOG(ERROR, "No ACK for AT command!\n");
            break;
        case IACK:
            CODEC_LOG(ERROR, "Invalid ACK for AT command!\n");
            break;
        default:
            CODEC_LOG(ERROR, "Unknown ACK for AT command!\n");
            break;
        }
        Thread::wait(osWaitForever);
    }

    return 0;
}

int32_t rda58xx::setAtHandler(int32_t (*handler)(int32_t status))
{
    if (NULL != handler) {
        atHandler = handler;
        return 0;
    } else {
        CODEC_LOG(ERROR, "Handler is NULL!\n");
        return -1;
    }
}

void rda58xx::rx_handler(void)
{
    uint8_t count = (RDA_UART1->FSR >> 9) & 0x7F;

    count = (count <= (_rx_buffer_size - _rx_idx)) ? count : (_rx_buffer_size - _rx_idx);
    while (count--) {
        tmpBuffer[tmpIdx] = (uint8_t) (RDA_UART1->RBR & 0xFF);
        tmpIdx++;
    }

    memcpy(&_rx_buffer[_rx_idx], tmpBuffer, tmpIdx);
    _rx_idx += tmpIdx;

    switch (_status) {
    case RECORDING:
        if (_rx_buffer_size == _rx_idx) {
            memcpy(&_rx_buffer[_rx_buffer_size], &_rx_buffer[0], _rx_buffer_size);
            _rx_buffer_status = FULL;
            _rx_idx = 0;
        }
        break;
        
    case STOP_RECORDING:
#ifdef RDA58XX_DEBUG
        mbed_error_printf("%d\n", _rx_idx);
#endif
        if ((_rx_idx >= 4) && (_rx_buffer[_rx_idx - 1] == '\n')) {
            memcpy(res_str, &_rx_buffer[_rx_idx - 4], 4);
            res_str[4] = '\0';
            if (strstr(RX_VALID_RSP2, (char *)res_str) != NULL) {
                _atr++;
                if (_atr == _ats) {
                    _rxsem.release();
                }
                _at_status = VACK;
                _rx_idx = 0;
                _status = PLAY;
#ifdef RDA58XX_DEBUG
                mbed_error_printf("%s\n", res_str);
#endif
            }
        }
        if (_rx_buffer_size == _rx_idx) {
            memcpy(&_rx_buffer[0], &_rx_buffer[_rx_buffer_size - 10], 10);
            _rx_idx = 10;
        }
        break;

    case UNREADY:
        if ((_rx_idx >= 6) && ('\n' == _rx_buffer[_rx_idx - 1])) {
            memcpy(res_str, &_rx_buffer[_rx_idx - 6], 6);
            res_str[6] = '\0';
            if (strstr(CODEC_READY_RSP, (char *)res_str) != NULL) {
                _ready = true;
                _power_on = true;
                _rx_idx = 0;
                _status = STOP;
#ifdef RDA58XX_DEBUG
                mbed_error_printf("%s\n", res_str);
#endif
            } else if (strstr(AST_POWERON_RSP, (char *)res_str) != NULL) {
                _power_on = true;
                _rx_idx = 0;
#ifdef RDA58XX_DEBUG
                mbed_error_printf("%s\n", res_str);
#endif                
            } else if (strstr(RX_VALID_RSP, (char *)res_str) != NULL) {
                if (_with_parameter) {
                    _parameter.bufferSize = ((_rx_idx - 10) < 64) ? (_rx_idx - 10) : 64;
                    memcpy(&_parameter.buffer[0], &_rx_buffer[2], _parameter.bufferSize);
                }
                _atr++;
                if (_atr == _ats) {
                    _rxsem.release();
                }
                _at_status = VACK;
#ifdef RDA58XX_DEBUG
                mbed_error_printf("%s\n", res_str);
#endif
                _rx_idx = 0;
            } else {
#if 1
                _rx_buffer[_rx_idx] = '\0';
                mbed_error_printf("%s\n", _rx_buffer);
#else
                for (uint32_t i = 0; i < _rx_idx; i++) {
                    mbed_error_printf("%02X#", _rx_buffer[i]);
                }
#endif
            }
        }
        break;

    case MODE_SWITCHING:
        if ((_rx_idx >= 4) && ('\n' == _rx_buffer[_rx_idx - 1])) {
            memcpy(res_str, &_rx_buffer[_rx_idx - 4], 4);
            res_str[4] = '\0';
            if (strstr(RX_VALID_RSP2, (char *)res_str) != NULL) {
                _atr++;
                if (_atr == _ats) {
                    _rxsem.release();
                }
                _at_status = VACK;
                _status = UNREADY;
#ifdef RDA58XX_DEBUG
                mbed_error_printf("%s\n", res_str);
#endif
                _rx_idx = 0;
            } else {
#if 1
                _rx_buffer[_rx_idx] = '\0';
                mbed_error_printf("%s", _rx_buffer);
#else
                for (uint32_t i = 0; i < _rx_idx; i++) {
                    mbed_error_printf("%02X#", _rx_buffer[i]);
                }
#endif
                _at_status = IACK;
                _status = STOP;
            }
        }
        break;
    
    default:
        if ((_rx_idx >= 4) && ('\n' == _rx_buffer[_rx_idx - 1])) {
            memcpy(res_str, &_rx_buffer[_rx_idx - 4], 4);
            res_str[4] = '\0';
            if (strstr(RX_VALID_RSP2, (char *)res_str) != NULL) {
                if (_with_parameter) {
                    _parameter.bufferSize = ((_rx_idx - 10) < 64) ? (_rx_idx - 10) : 64;
                    memcpy(&_parameter.buffer[0], &_rx_buffer[2], _parameter.bufferSize);
                }
                _atr++;
                if (_atr == _ats) {
                    _rxsem.release();
                }
                _at_status = VACK;
#ifdef RDA58XX_DEBUG
                mbed_error_printf("%s\n", res_str);
#endif
                _rx_idx = 0;
            } else {
#if 1
                _rx_buffer[_rx_idx] = '\0';
                mbed_error_printf("%s", _rx_buffer);
#else
                for (uint32_t i = 0; i < _rx_idx; i++) {
                    mbed_error_printf("%02X#", _rx_buffer[i]);
                }
#endif
                _at_status = IACK;
            }
        }
        break;
    }

    tmpIdx = 0;

    if (_rx_idx >= _rx_buffer_size)
        _rx_idx = 0;
}

void rda58xx::tx_handler(void)
{
    if ((_tx_buffer != NULL) || (_tx_buffer_size != 0)) {
        uint8_t n = ((_tx_buffer_size - _tx_idx) < 64) ? (_tx_buffer_size - _tx_idx) : 64;
        while (n--) {
            RDA_UART1->THR = _tx_buffer[_tx_idx];
            _tx_idx++;
        }

        if (_tx_idx == _tx_buffer_size) {
            RDA_UART1->IER = (RDA_UART1->IER) & (~(0x01L << 1));//disable UART1 TX interrupt
            _tx_buffer = NULL;
            _tx_buffer_size = 0;
            _tx_idx = 0;
        }
    } else {
        RDA_UART1->IER = (RDA_UART1->IER) & (~(0x01L << 1));//disable UART1 TX interrupt
        _tx_buffer_size = 0;
        _tx_idx = 0;
        return;
    }
}

