/* mbed Microcontroller Library
 * Copyright (c) 2006-2012 ARM Limited
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in
 * all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
 * SOFTWARE.
 */
/* Introduction
 * ------------
 * SD and MMC cards support a number of interfaces, but common to them all
 * is one based on SPI. This is the one I'm implmenting because it means
 * it is much more portable even though not so performant, and we already
 * have the mbed SPI Interface!
 *
 * The main reference I'm using is Chapter 7, "SPI Mode" of:
 *  http://www.sdcard.org/developers/tech/sdcard/pls/Simplified_Physical_Layer_Spec.pdf
 *
 * SPI Startup
 * -----------
 * The SD card powers up in SD mode. The SPI interface mode is selected by
 * asserting CS low and sending the reset command (CMD0). The card will
 * respond with a (R1) response.
 *
 * CMD8 is optionally sent to determine the voltage range supported, and
 * indirectly determine whether it is a version 1.x SD/non-SD card or
 * version 2.x. I'll just ignore this for now.
 *
 * ACMD41 is repeatedly issued to initialise the card, until "in idle"
 * (bit 0) of the R1 response goes to '0', indicating it is initialised.
 *
 * You should also indicate whether the host supports High Capicity cards,
 * and check whether the card is high capacity - i'll also ignore this
 *
 * SPI Protocol
 * ------------
 * The SD SPI protocol is based on transactions made up of 8-bit words, with
 * the host starting every bus transaction by asserting the CS signal low. The
 * card always responds to commands, data blocks and errors.
 *
 * The protocol supports a CRC, but by default it is off (except for the
 * first reset CMD0, where the CRC can just be pre-calculated, and CMD8)
 * I'll leave the CRC off I think!
 *
 * Standard capacity cards have variable data block sizes, whereas High
 * Capacity cards fix the size of data block to 512 bytes. I'll therefore
 * just always use the Standard Capacity cards with a block size of 512 bytes.
 * This is set with CMD16.
 *
 * You can read and write single blocks (CMD17, CMD25) or multiple blocks
 * (CMD18, CMD25). For simplicity, I'll just use single block accesses. When
 * the card gets a read command, it responds with a response token, and then
 * a data token or an error.
 *
 * SPI Command Format
 * ------------------
 * Commands are 6-bytes long, containing the command, 32-bit argument, and CRC.
 *
 * +---------------+------------+------------+-----------+----------+--------------+
 * | 01 | cmd[5:0] | arg[31:24] | arg[23:16] | arg[15:8] | arg[7:0] | crc[6:0] | 1 |
 * +---------------+------------+------------+-----------+----------+--------------+
 *
 * As I'm not using CRC, I can fix that byte to what is needed for CMD0 (0x95)
 *
 * All Application Specific commands shall be preceded with APP_CMD (CMD55).
 *
 * SPI Response Format
 * -------------------
 * The main response format (R1) is a status byte (normally zero). Key flags:
 *  idle - 1 if the card is in an idle state/initialising
 *  cmd  - 1 if an illegal command code was detected
 *
 *    +-------------------------------------------------+
 * R1 | 0 | arg | addr | seq | crc | cmd | erase | idle |
 *    +-------------------------------------------------+
 *
 * R1b is the same, except it is followed by a busy signal (zeros) until
 * the first non-zero byte when it is ready again.
 *
 * Data Response Token
 * -------------------
 * Every data block written to the card is acknowledged by a byte
 * response token
 *
 * +----------------------+
 * | xxx | 0 | status | 1 |
 * +----------------------+
 *              010 - OK!
 *              101 - CRC Error
 *              110 - Write Error
 *
 * Single Block Read and Write
 * ---------------------------
 *
 * Block transfers have a byte header, followed by the data, followed
 * by a 16-bit CRC. In our case, the data will always be 512 bytes.
 *
 * +------+---------+---------+- -  - -+---------+-----------+----------+
 * | 0xFE | data[0] | data[1] |        | data[n] | crc[15:8] | crc[7:0] |
 * +------+---------+---------+- -  - -+---------+-----------+----------+
 */
#include "USBMSD_SD.h"
#include "mbed_debug.h"
#include "sdmmc.h"

#define SD_DBG             1
USBMSD_SD::USBMSD_SD(PinName clk, PinName cmd, PinName d0,
                     PinName d1, PinName d2, PinName d3) :
    USBMSD (0x8889, 0x1e04, 0x0001) {
    _is_initialized = 0;
    rda_sdmmc_init(clk, cmd, d0, d1, d2, d3);
    debug_if(SD_DBG, "USBMSD_SD::USBMSD_SD 1\r\n");
    connect();
}

int USBMSD_SD::disk_initialize() {
    _is_initialized = rda_sdmmc_open();
    if (_is_initialized == 0) {
        debug("Fail to initialize card\n");
        return 1;
    }
    debug_if(SD_DBG, "init card = %d\n", _is_initialized);
    _sectors = _sd_sectors();

    return 0;
}

int USBMSD_SD::disk_write(const uint8_t *buffer, uint64_t block_number, uint8_t count) {
    int err;
    if (!_is_initialized) {
        return -1;
    }
    err = rda_sdmmc_write_blocks((unsigned char *)buffer, block_number, count);
    if(err != 0) {
        _is_initialized = 0;
    }
    return 0;
}

int USBMSD_SD::disk_read(uint8_t *buffer, uint64_t block_number, uint8_t count) {
    int err;
    if (!_is_initialized) {
        return -1;
    }
    err = rda_sdmmc_read_blocks(buffer, block_number, count);
    if(err != 0) {
        _is_initialized = 0;
    }
    return 0;
}

int USBMSD_SD::disk_status() {
    /* FATFileSystem::disk_status() returns 0 when initialized */
    int ret = _is_initialized ? 0 : 1;
    return ret;
}

int USBMSD_SD::disk_check()
{
    int err = 0;
    if(0 == _is_initialized) {
        err = disk_initialize();
    } else {
        unsigned char buf[512];
        err = rda_sdmmc_read_blocks(buf, 0, 1);
        if(0 != err) {
            _is_initialized = 0;
        }
    }
    return err;
}

int USBMSD_SD::disk_sync() {
    return 0;
}

uint64_t USBMSD_SD::disk_sectors() {
    return _sectors;
}

uint64_t USBMSD_SD::_sd_sectors() {
    uint32_t blocks;
    int csd_structure;

    /* Get CSD info */
    csd_structure = rda_sdmmc_get_csdinfo((unsigned long *)(&blocks));

    switch (csd_structure) {
        case 0:
            cdv = 512;
            debug_if(SD_DBG, "\n\rSDCard\n\rsectors: %lld\n\r", blocks);
            break;

        case 1:
            cdv = 1;
            debug_if(SD_DBG, "\n\rSDHC Card \n\rsectors: %lld\n\r", blocks);
            break;

        default:
            debug("CSD struct unsupported\r\n");
            return 0;
    };
    return blocks;

}
