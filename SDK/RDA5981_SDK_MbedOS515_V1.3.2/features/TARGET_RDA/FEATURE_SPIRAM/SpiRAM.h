/*
 * File : SpiRAM.h
 * Dscr : Headers for SPI RAM "IPS3204J".
 */

#ifndef SPI_RAM_H
#define SPI_RAM_H

#include "mbed.h"
#include "spi_api.h"

#define INVALID_BYTE    (0xFFU)
#define INVALID_WORD    (0xFFFFFFFFUL)

typedef enum {
    SPIRAM_MODE_BYTE = 1,
    SPIRAM_MODE_WORD = 4
} SPIRAM_MODE_T;

typedef enum {
    SPIRAM_RET_SUCCESS      = 0,
    SPIRAM_RET_ERR_INPUT    = -1
} SPIRAM_RET_CODE_T;

class SpiRAM
{
public:
    /* Initialize and specify the SPI pins */
    SpiRAM(PinName mosi, PinName miso, PinName sclk, PinName cs, SPIRAM_MODE_T mode = SPIRAM_MODE_WORD);

    /* Set SPIRAM to byte/word mode */
    void setMode(SPIRAM_MODE_T mode);

    /* Read a single byte from address */
    uint8_t readByte(uint32_t address);

    /* Write a single byte to address */
    void writeByte(uint32_t address, uint8_t data_byte);

    /* Read a single word from address */
    uint32_t readWord(uint32_t address);

    /* Write a single word to address */
    void writeWord(uint32_t address, uint32_t data_word);

    /* Read several bytes starting at address and of length into a char buffer */
    SPIRAM_RET_CODE_T readBuffer(uint32_t address, void *buffer, uint32_t length);

    /* Write several bytes starting at address and of length from a char buffer */
    SPIRAM_RET_CODE_T writeBuffer(uint32_t address, void *buffer, uint32_t length);

    /* Acquire exclusive access to this SpiRAM device */
    virtual void lock(void);

    /* Release exclusive access to this SpiRAM device */
    virtual void unlock(void);

private:
    spi_t _spi;
    DigitalOut _cs;
    SPIRAM_MODE_T _mode;
    static SingletonPtr<PlatformMutex> _mutex;

    void readBufferRaw(uint32_t address, char *buffer, uint32_t length);
    void writeBufferRaw(uint32_t address, char *buffer, uint32_t length);
    void readBufferRaw(uint32_t address, int *buffer, uint32_t length);
    void writeBufferRaw(uint32_t address, int *buffer, uint32_t length);
    void resetDevice(void);
    void setCmdAddress(uint8_t cmd, uint32_t address);
};

#endif

