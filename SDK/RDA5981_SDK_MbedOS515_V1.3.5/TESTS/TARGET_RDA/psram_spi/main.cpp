#include "mbed.h"
#include "SpiRAM.h"

//#define TMP_BORDER  0x003FF
#define TMP_BORDER  0x0040F
//#define TMP_BORDER  0x0041F
#define TMP_LEN     10
#define WORD_LEN    32

const uint32_t ramSize = 0x3FFFFF;           // 4M x 8 bit
char buffer[] = {"0123456789ABCDEF"};
int s4_buf[WORD_LEN];
int s4_buf1[WORD_LEN] = {0};
char tmpBuf[1024] = {0};
/* SPI Pins: mosi, miso, sclk, ssel */
SpiRAM sRam(PD_2, PD_3, PD_0, PD_1);

void sRamDump(uint32_t addr, uint32_t len);

int main()
{
    uint32_t x;

    printf("Start PSRAM test...\r\n");

    /* Test byte mode */
    printf("\r\nSet byte mode");
    sRam.setMode(SPIRAM_MODE_BYTE);
    printf("\r\nWrite Memory with Buffer.");
    printf("\r\nTmpBorder=%08X, TmpLen=%d", TMP_BORDER, TMP_LEN);
    for(x = 0; x < (TMP_BORDER - TMP_LEN); x += TMP_LEN) {
        sRam.writeBuffer(x, buffer, TMP_LEN);
    }

    printf("\r\nDump Memory 1:");
    sRamDump(0, 100);
    sRamDump(TMP_BORDER - 100, 100);

    printf("\r\nWrite byte.");
    for(x = 0x3FFBF8; x <= 0x3FFFFF; x++) {
        sRam.writeByte(x, (uint8_t)(x & 0xFFU));
    }

    printf("\r\nDump Memory 2:");
    sRam.setMode(SPIRAM_MODE_BYTE);
    sRamDump(0x3FFBF8, 100);
    sRamDump(ramSize - 0x7F, 0x80);
    printf("\r\n");

    /* Test word mode */
    printf("\r\nSet word mode");
    sRam.setMode(SPIRAM_MODE_WORD);
    for(x = 0; x < WORD_LEN; x++) {
        s4_buf[x] = 0xAABBCCDD;
    }
    printf("\r\nWrite buffer.");
    sRam.writeBuffer(0x2FFF80, s4_buf, (WORD_LEN << 2));

    printf("\r\nRead buffer.");
    sRam.readBuffer(0x2FFF80, s4_buf1, (WORD_LEN << 2));
    for(x = 0; x < WORD_LEN; x++) {
        if(s4_buf[x] != s4_buf1[x]) {
            printf("s4_buf[%d]=%08X,s4_buf1[%d]=%08X\r\n", (int)x, s4_buf[x], (int)x, s4_buf1[x]);
            break;
        }
    }
    printf("\r\nCheck buffer: %s", (x == WORD_LEN) ? "SUCCESS" : "FAIL");

    printf("\r\nDump Memory 3:");
    sRam.setMode(SPIRAM_MODE_BYTE);  // set byte mode to dump SpiRAM
    sRamDump(0x2FFF80, (WORD_LEN << 2));
    printf("\r\nDone.\r\n");

    Thread::wait(osWaitForever);
}

void sRamDump(uint32_t addr, uint32_t len)
{
    uint32_t i;
    /* Block to 16 */
    addr &= ~0x0FUL;
    len = (len + 0x0FUL) & ~0x0FUL;

    for(i = 0; i < len; i++) {
        uint8_t b = sRam.readByte(addr);
        if(addr % 16 == 0) {
            printf("\r\n%08X:", (unsigned int)addr);
        }
        printf(" %02X", (unsigned int)b);
        addr++;
    }
    printf("\r\n");
}

