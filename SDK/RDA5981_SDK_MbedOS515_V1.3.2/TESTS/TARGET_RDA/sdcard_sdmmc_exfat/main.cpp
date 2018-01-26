#include "mbed.h"
#include "SDMMCFileSystem.h"

int main()
{
    /* SDMMC Pins: clk, cmd, d0, d1, d2, d3 */
    //SDMMCFileSystem sdc(PB_9, PB_0, PB_6, PB_7, PC_0, PC_1, "sdc");  // for old EVB
    SDMMCFileSystem sdc(PB_9, PB_0, PB_3, PB_7, PC_0, PC_1, "sdc");  // for RDA5991H_HDK_V1.0
    const char *file_name0 = "/sdc/mydir/sdctest.txt";
    const char *file_name1 = "/sdc/mydir/sdcmultitest.txt";
    /* Add by Lattner for exfat */
    const char *file_name2 = "/sdc/en_sql_server_2012_enterprise_edition_x86_x64_dvd_813294.iso";
    const char *file_name3 = "/sdc/big_file";
    
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
    printf("buf1 is %s\r\n", buf);    
    fclose(fp);
    
    /* Add by Lattner for testing exfat */
    fp = fopen(file_name2, "rb");
    if (NULL == fp) {
        error("Could not open %s for binary read!\r\n", file_name2);
    }
    /* Skip the initial part */
    if (fseek(fp, 1024*1024*10, SEEK_SET) != 0)
        error("fseek1 goes wrong\r\n");
    if (fread(buf, sizeof(char), 1599, fp) != 1599*sizeof(char))
        error("error happened when read %s\r\n", file_name2);
    buf[1599] = '\0';
    printf("buf2 is %s\r\n", buf);        

    /* Skip the    ending  part */
    if (fseek(fp, -1024*1024*10, SEEK_END) != 0)
        error("fseek goes wrong\r\n");
    if (fread(buf, sizeof(char), 1599, fp) != 1599*sizeof(char))
        printf("error happened when read %s\r\n", file_name2);
    buf[1599] = '\0';
    fclose(fp);

    fp = fopen(file_name3, "ab+");    
    if (NULL == fp) {
        error("Could not open %s for binary write!\r\n", file_name3);
    }    
    if (fwrite(buf, sizeof(char), 1600, fp) != 1600*sizeof(char))
        error("error happened when write %s\r\n", file_name3);
    
    /*
    fseek(fp, 0, SEEK_SET);
    if (fread(buf, sizeof(char), 1600, fp) != 1600*sizeof(char))
        printf("fread error \n!");
    printf("buf3 is %s\r\n", buf);
    if (fseek(fp, -1600L, SEEK_END) != 0)
        printf("fseek error\n");
    printf("read size = %u\n", fread(buf, sizeof(char), 1599, fp));
    buf[1599] = '\0';
    printf("buf4 is %s\r\n", buf);
    */

    /*
    for (idx = 0; idx < 4; ++idx) {
        if (fseek(fp, 2097152*512, SEEK_CUR) != 0) // file position moves 1G
            error("fseek of 2097152 went wrong\r\n");
        // if (fseek(fp, 2147483648, SEEK_SET) != 0)
        //     error("fseek went wrong\r\n");

        printf("ftell current position is %x\r\n", (unsigned)ftell(fp)); 
    */
    /*
     for (idx = 0; idx < 4; ++idx) {
          printf("ftell current position is %x\r\n", (unsigned)ftell(fp)); 
          if (fwrite(buf, sizeof(char), 1600, fp) != 1600*sizeof(char))
               printf("error happened when write %s\r\n", file_name3);
         }
     */
    fclose(fp);
    
    printf("End of SD Card Test...\r\n");

    while(true) {
    }
}
