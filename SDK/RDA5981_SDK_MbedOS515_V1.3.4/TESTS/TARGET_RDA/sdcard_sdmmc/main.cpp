#include "mbed.h"
#include "SDMMCFileSystem.h"

int main()
{
    /* SDMMC Pins: clk, cmd, d0, d1, d2, d3 */
    //SDMMCFileSystem sdc(PB_9, PB_0, PB_6, PB_7, PC_0, PC_1, "sdc");  // for old EVB
    SDMMCFileSystem sdc(PB_9, PB_0, PB_3, PB_7, PC_0, PC_1, "sdc");  // for RDA5991H_HDK_V1.0
    const char *file_name0 = "/sdc/mydir/sdctest.txt";
    const char *file_name1 = "/sdc/mydir/sdcmultitest.txt";
    const char *wrstr = "Hello world!\r\nSD Card\r\n";
    char rdstr[64] = {'\0'};
    char buf[1600] = {0};
    int idx = 0;
    FILE *fp = NULL;

    printf("Start SD Card Test...\r\n\r\n");

    mkdir("/sdc/mydir", 0777);

    fp = fopen(file_name0, "w+");
    if(NULL == fp) {
        error("Could not open file for write!\r\n");
    }

    fprintf(fp, wrstr);
    printf("Write text to file: %s\r\n%s\r\n", file_name0, wrstr);

    /* Set fp at file head */
    fseek(fp, 0L, SEEK_SET);

    fread(rdstr, sizeof(char), 64, fp);
    printf("Read text from file: %s\r\n%s\r\n", file_name0, rdstr);
    fclose(fp);

    for(idx = 0; idx <1600; idx++) {
        buf[idx] = '0' + (idx % 40);
    }

    fp = fopen(file_name1, "wb+");
    if(NULL == fp) {
        error("Could not open file for binary write!\r\n");
    }

    for(idx = 0; idx <100; idx++) {
        fwrite(buf, sizeof(char), 1600, fp);
        printf("%d,", idx);
    }
    printf("\r\nWrite 1600 chars to file: %s\r\n",file_name1);

    fseek(fp, 0L, SEEK_SET);

    fread(buf, sizeof(char), 1600, fp);
    printf("Read 1600 chars from file: %s\r\n",file_name1);
    fclose(fp);

    printf("End of SD Card Test...\r\n");

    while(true) {
    }
}
