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
#ifndef MBED_SDMMCFILESYSTEM_H
#define MBED_SDMMCFILESYSTEM_H

#include "mbed.h"
#include "FATFileSystem.h"
#include <stdint.h>

/** Access the filesystem on an SD Card using SDMMC
 *
 * @code
 * #include "mbed.h"
 * #include "SDMMCFileSystem.h"
 *
 * SDMMCFileSystem sd(p5, p6, p7, p12, "sd"); // clk, cmd, d0, d1, d2, d3
 *
 * int main() {
 *     FILE *fp = fopen("/sd/myfile.txt", "w");
 *     fprintf(fp, "Hello World!\n");
 *     fclose(fp);
 * }
 */
class SDMMCFileSystem : public FATFileSystem {
public:

    /** Create the File System for accessing an SD Card using SPI
     *
     * @param clk  SDMMC clk pin connected to SD Card
     * @param cmd  SDMMC cmd pin conencted to SD Card
     * @param d0   SDMMC d0 pin connected to SD Card
     * @param d1   SDMMC d1 pin connected to SD Card
     * @param d2   SDMMC d2 pin connected to SD Card
     * @param d3   SDMMC d3 pin connected to SD Card
     * @param name The name used to access the virtual filesystem
     */
    SDMMCFileSystem(PinName clk, PinName cmd, PinName d0,
        PinName d1, PinName d2, PinName d3, const char* name);
    virtual int disk_initialize();
    virtual int disk_status();
    virtual int disk_read(uint8_t* buffer, uint32_t block_number, uint32_t count);
    virtual int disk_write(const uint8_t* buffer, uint32_t block_number, uint32_t count);
    virtual int disk_sync();
    virtual uint32_t disk_sectors();

    int disk_check();

protected:

    uint32_t _sd_sectors();
    uint32_t _sectors;

    int cdv;
    int _is_initialized;
};

#endif
