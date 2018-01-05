/*
 * File : SpiRAM.c
 * Dscr : Functions for SPI RAM "IPS3204J".
 */
#include "SpiRAM.h"

/* Macros */
//#define TOGGLE_SPI_ENDIANNESS
#define REVERSE_INT32(i)    ((((i) << 24) & 0xFF000000UL) \
                           | (((i) <<  8) & 0x00FF0000UL) \
                           | (((i) >>  8) & 0x0000FF00UL) \
                           | (((i) >> 24) & 0x000000FFUL))

#define RAM_SPI_FREQ        20000000
#define RAM_PAGE_SIZE       1024U
#define NEXT_PAGE_ADDR(a)   (((a) + RAM_PAGE_SIZE) & ~(RAM_PAGE_SIZE - 1U))

#define SPIRAM_CMD_WRITE    (0x02U)
#define SPIRAM_CMD_READ     (0x03U)
#define SPIRAM_CMD_RST_EN   (0x66U)
#define SPIRAM_CMD_RST      (0x99U)
#define SPIRAM_CMD_READ_ID  (0x9FU)

SingletonPtr<PlatformMutex> SpiRAM::_mutex;

/* Constructor */
SpiRAM::SpiRAM(PinName mosi, PinName miso, PinName sclk, PinName cs, SPIRAM_MODE_T mode):
        _cs(cs), _mode(mode)
{
    spi_init(&_spi, mosi, miso, sclk, NC);
    spi_format(&_spi, 8, 1, 0);  // ensure 8-bit mode before resetDevice()
    spi_frequency(&_spi, RAM_SPI_FREQ);
    resetDevice();
    if(_mode == SPIRAM_MODE_WORD) {
        spi_format(&_spi, 32, 1, 0);
    }
}

void SpiRAM::setMode(SPIRAM_MODE_T mode)
{
    int bits;
    _mode = mode;
    bits = (_mode == SPIRAM_MODE_WORD) ? 32 : 8;
    spi_format(&_spi, bits, 1, 0);
}

uint8_t SpiRAM::readByte(uint32_t address)
{
    uint8_t data_byte;
    if(_mode == SPIRAM_MODE_BYTE) {
        readBufferRaw(address, (char *)(&data_byte), 1U);
    } else {
        data_byte = (uint8_t)INVALID_BYTE;
    }
    return data_byte;
}

void SpiRAM::writeByte(uint32_t address, uint8_t data_byte)
{
    if(_mode == SPIRAM_MODE_BYTE) {
        writeBufferRaw(address, (char *)(&data_byte), 1U);
    }
}

uint32_t SpiRAM::readWord(uint32_t address)
{
    uint32_t data_word;
    if(_mode == SPIRAM_MODE_WORD) {
        readBufferRaw(address, (int *)(&data_word), 1U);
    } else {
        data_word = (uint32_t)INVALID_WORD;
    }
    return data_word;
}

void SpiRAM::writeWord(uint32_t address, uint32_t data_word)
{
    if(_mode == SPIRAM_MODE_WORD) {
        writeBufferRaw(address, (int *)(&data_word), 1U);
    }
}

SPIRAM_RET_CODE_T SpiRAM::readBuffer(uint32_t address, void *buffer, uint32_t length)
{
    uint32_t ofst_len = 0U, delta_len;
    if(_mode == SPIRAM_MODE_WORD) {
        int *buf_int = (int *)buffer;
        if((address & 0x03UL) || ((uint32_t)buffer & 0x03UL) || (length & 0x03UL)) {
            return SPIRAM_RET_ERR_INPUT;
        }
        for(; NEXT_PAGE_ADDR(address + ofst_len) < (address + length); ofst_len += delta_len) {
            delta_len = NEXT_PAGE_ADDR(address + ofst_len) - (address + ofst_len);
            readBufferRaw(address + ofst_len, buf_int + (ofst_len >> 2), delta_len);
        }
        if(ofst_len < length) {
            readBufferRaw(address + ofst_len, buf_int + (ofst_len >> 2), length - ofst_len);
        }
    } else {
        char *buf_char = (char *)buffer;
        for(; NEXT_PAGE_ADDR(address + ofst_len) < (address + length); ofst_len += delta_len) {
            delta_len = NEXT_PAGE_ADDR(address + ofst_len) - (address + ofst_len);
            readBufferRaw(address + ofst_len, buf_char + ofst_len, delta_len);
        }
        if(ofst_len < length) {
            readBufferRaw(address + ofst_len, buf_char + ofst_len, length - ofst_len);
        }
    }
    return SPIRAM_RET_SUCCESS;
}

SPIRAM_RET_CODE_T SpiRAM::writeBuffer(uint32_t address, void *buffer, uint32_t length)
{
    uint32_t ofst_len = 0U, delta_len;
    if(_mode == SPIRAM_MODE_WORD) {
        int *buf_int = (int *)buffer;
        if((address & 0x03UL) || ((uint32_t)buffer & 0x03UL) || (length & 0x03UL)) {
            return SPIRAM_RET_ERR_INPUT;
        }
        for(; NEXT_PAGE_ADDR(address + ofst_len) < (address + length); ofst_len += delta_len) {
            delta_len = NEXT_PAGE_ADDR(address + ofst_len) - (address + ofst_len);
            writeBufferRaw(address + ofst_len, buf_int + (ofst_len >> 2), delta_len);
        }
        if(ofst_len < length) {
            writeBufferRaw(address + ofst_len, buf_int + (ofst_len >> 2), length - ofst_len);
        }
    } else {
        char *buf_char = (char *)buffer;
        for(; NEXT_PAGE_ADDR(address + ofst_len) < (address + length); ofst_len += delta_len) {
            delta_len = NEXT_PAGE_ADDR(address + ofst_len) - (address + ofst_len);
            writeBufferRaw(address + ofst_len, buf_char + ofst_len, delta_len);
        }
        if(ofst_len < length) {
            writeBufferRaw(address + ofst_len, buf_char + ofst_len, length - ofst_len);
        }
    }
    return SPIRAM_RET_SUCCESS;
}

void SpiRAM::readBufferRaw(uint32_t address, char *buffer, uint32_t length)
{
    uint32_t idx;
    _cs = 0;
    setCmdAddress(SPIRAM_CMD_READ, address);
    for(idx = 0U; idx < length; idx++) {
        buffer[idx] = (char)(spi_master_write(&_spi, 0x00));
    }
    _cs = 1;
    /* Force dummy transfer for command termination */
    spi_master_write(&_spi, 0x00);
}

void SpiRAM::writeBufferRaw(uint32_t address, char *buffer, uint32_t length)
{
    uint32_t idx;
    _cs = 0;
    setCmdAddress(SPIRAM_CMD_WRITE, address);
    for(idx = 0U; idx < length; idx++) {
        spi_master_write(&_spi, (int)(buffer[idx]));
    }
    _cs = 1;
    /* Force dummy transfer for command termination */
    spi_master_write(&_spi, 0x00);
}

void SpiRAM::readBufferRaw(uint32_t address, int *buffer, uint32_t length)
{
    uint32_t idx;
    length = length >> 2;  // calculate the number of words
    _cs = 0;
    setCmdAddress(SPIRAM_CMD_READ, address);
    for(idx = 0U; idx < length; idx++) {
#if !defined(TOGGLE_SPI_ENDIANNESS)
        buffer[idx] = spi_master_write(&_spi, 0x00);
#else  /* !TOGGLE_SPI_ENDIANNESS */
        int data_word = spi_master_write(&_spi, 0x00);
        buffer[idx] = REVERSE_INT32(data_word);
#endif /* !TOGGLE_SPI_ENDIANNESS */
    }
    _cs = 1;
    /* Force dummy transfer for command termination */
    spi_master_write(&_spi, 0x00);
}

void SpiRAM::writeBufferRaw(uint32_t address, int *buffer, uint32_t length)
{
    uint32_t idx;
    length = length >> 2;  // calculate the number of words
    _cs = 0;
    setCmdAddress(SPIRAM_CMD_WRITE, address);
    for(idx = 0U; idx < length; idx++) {
#if !defined(TOGGLE_SPI_ENDIANNESS)
        spi_master_write(&_spi, buffer[idx]);
#else  /* !TOGGLE_SPI_ENDIANNESS */
        spi_master_write(&_spi, REVERSE_INT32(buffer[idx]));
#endif /* !TOGGLE_SPI_ENDIANNESS */
    }
    _cs = 1;
    /* Force dummy transfer for command termination */
    spi_master_write(&_spi, 0x00);
}

void SpiRAM::resetDevice(void)
{
    /* Reset cmd must be in SPI byte mode */
    _cs = 1;            /* Force CS high */
    spi_master_write(&_spi, 0xFF);   /* Dummy write */
    _cs = 0;                        /* Reset enable command start */
    spi_master_write(&_spi, SPIRAM_CMD_RST_EN);  /* Reset eneble command */
    _cs = 1;                        /* Reset enable command finish */
    spi_master_write(&_spi, 0xFF);               /* Dummy transfer for command termination of reset command */
    _cs = 0;                    /* Reset command start */
    spi_master_write(&_spi, SPIRAM_CMD_RST); /* Reset command */
    _cs = 1;                    /* Reset command finish */
    spi_master_write(&_spi, 0xFF);           /* Dummy transfer for command termination of reset command */
}

void SpiRAM::setCmdAddress(uint8_t cmd, uint32_t address)
{
    /* Address must be big-endian */
    if(_mode == SPIRAM_MODE_WORD) {
        spi_master_write(&_spi, (int)(((uint32_t)cmd << 24) | (address & 0x00FFFFFFUL)));
    } else {
        spi_master_write(&_spi, (int)cmd);
        spi_master_write(&_spi, (int)((address >> 16) & 0xFFUL));
        spi_master_write(&_spi, (int)((address >>  8) & 0xFFUL));
        spi_master_write(&_spi, (int)( address        & 0xFFUL));
    }
}

void SpiRAM::lock()
{
    _mutex->lock();
}

void SpiRAM::unlock()
{
    _mutex->unlock();
}

