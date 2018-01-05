#include "mbed.h"
#include "WiFiStackInterface.h"
#include "rda5991h_wland.h"
#include "rda5981_flash.h"
#include "wland_rf.h"

#ifdef HI_FLYING
wland_tx_power_t tx_power;//tx power, deprecated, only for hiflying
#endif
unsigned char xtal_cal = 0x40;//range 0~0x7F, reg 0xDA
unsigned char tx_power_phy[2];//[0]g/n [1]b
unsigned char tx_power_phy_gn = 0x45;//range 0x25~0x64, reg 0x11F
unsigned char tx_power_phy_b = 0x45;//range 0x15~0x54, reg 0x120
unsigned short tx_power_rf[WLAND_CHANNEL_NUM] = { //reg 0x8A
    0x69a0,
    0x69a0,
    0x69a0,
    0x69a0,
    0x69a0,
    0x6920,
    0x6920,
    0x6920,
    0x68a0,
    0x6820,
    0x6820,
    0x6820,
    0x6820,
    0x6820
};


int main()
{
    WiFiStackInterface wifi;
    int ret = 0;

    printf("Start rf config test...\r\n");

    wifi.connect(NULL, NULL, NULL, NSAPI_SECURITY_NONE);

    /* Write reg, it will take effect immediately */
    
    /* write xtal calibration, reg 0xDA, value range is 0~0x7F*/
    ret = wland_rf_write(WLAND_XTAL_CAL_REG, xtal_cal);
    if (ret < 0)
        printf("write xtal cal fail\r\n");
    else
        printf("write xtal cal succeed\r\n");
     
    /* write tx power phy gn, reg 0x11F, value range is 0x25~0x64*/
    ret = wland_phy_write(WLAND_TX_POWER_PHY_GN_REG, tx_power_phy_gn);
    if (ret < 0)
        printf("write tx power gn fail\r\n");
    else
        printf("write tx power gn succeed\r\n");
    
    /* write tx power phy b, reg 0x120, value range is 0x15~0x54*/
    ret = wland_rf_write(WLAND_TX_POWER_PHY_B_REG, tx_power_phy_b);
    if (ret < 0)
        printf("write tx power b fail\r\n");
    else
        printf("write tx power b succeed\r\n");

    /* write tx power rf, reg 0x8A */
    ret = wland_rf_write_all_channels(WLAND_TX_POWER_RF_REG, tx_power_rf, WLAND_CHANNEL_NUM);
    if (ret < 0)
        printf("write tx power rf fail\r\n");
    else
        printf("write tx power rf succeed\r\n");

    
    /* Write Flash */
#ifdef HI_FLYING
    /* initialize tx power, tx_power.b[0] is 11b-ch1's power */
    for (unsigned char i = 0; i < 14; i++) {
        tx_power.b[i] = 0x35;
        tx_power.g[i] = 0x4A;
        tx_power.n[i] = 0x45;
    }
    /* write tx power to flash */
    ret = rda5981_write_user_data((unsigned char *)&tx_power, sizeof(tx_power), RDA5991H_USER_DATA_FLAG_TX_POWER);
    if (ret < 0)
        printf("write tx power(deprecated) to flash fail\r\n");
    else
        printf("write tx power(deprecated) to flash succeed\r\n");
#endif
    
    /* write xtal calibration to flash, range is 0~0x7F*/
    ret = rda5981_write_user_data((unsigned char *)&xtal_cal, sizeof(xtal_cal), RDA5991H_USER_DATA_FLAG_XTAL_CAL);
    if (ret < 0)
        printf("write xtal cal to flash fail\r\n");
    else
        printf("write xtal cal to flash succeed\r\n");

    /* write tx power phy g/n to flash */
    ret = rda5981_write_user_data((unsigned char *)&tx_power_phy_gn, sizeof(tx_power_phy_gn), RDA5991H_USER_DATA_FLAG_TX_POWER_PHY_GN);
    if (ret < 0)
        printf("write tx power phy gn to flash fail\r\n");
    else
        printf("write tx power phy gn to flash succeed\r\n");

    /* write tx power phy b to flash */
    ret = rda5981_write_user_data((unsigned char *)&tx_power_phy_b, sizeof(tx_power_phy_b), RDA5991H_USER_DATA_FLAG_TX_POWER_PHY_B);
    if (ret < 0)
        printf("write tx power phy b to flash fail\r\n");
    else
        printf("write tx power phy b to flash succeed\r\n");

    /* write tx powerrf to flash */
    ret = rda5981_write_user_data((unsigned char *)&tx_power_rf, sizeof(tx_power_rf), RDA5991H_USER_DATA_FLAG_TX_POWER_RF);
    if (ret < 0)
        printf("write tx power rf to flash fail\r\n");
    else
        printf("write tx power rf to flash succeed\r\n");


    /* Update from Flash */
#ifdef HI_FLYING
    /* update tx power from flash, tx power stored in flash will be wrote in corresponding reg */
    ret = update_tx_power_from_flash();
    if (ret < 0)
        printf("update tx power(deprecated) from flash fail\r\n");
    else
        printf("update tx power(deprecated) from flash succeed\r\n");
#endif

    /* update xtal calibration from flash,xtal calibration stored in flash will be wrote in corresponding reg */
    ret = update_xtal_cal_from_flash();
    if (ret < 0)
        printf("update xtal cal from flash fail\r\n");
    else
        printf("update xtal cal from flash succeed\r\n");

    /* update tx power phy from flash, tx power phy g/n & b stored in flash will be wrote in corresponding reg */
    ret = update_tx_power_phy_from_flash();
    if (ret < 0)
        printf("update tx power phy from flash fail\r\n");
    else
        printf("update tx power phy from flash succeed\r\n");

    /* update tx power rf from flash, tx power rf stored in flash will be wrote in corresponding reg */
    ret = update_tx_power_rf_from_flash();
    if (ret < 0)
        printf("update tx power rf from flash fail\r\n");
    else
        printf("update tx power rf from flash succeed\r\n");


    /* Read Flash, almost same with Write Flash*/
    /* read xtal calibration from flash, value will be stored in xtal_cal*/
    ret = rda5981_read_user_data((unsigned char *)&xtal_cal, sizeof(xtal_cal), RDA5991H_USER_DATA_FLAG_XTAL_CAL);
    if (ret < 0)
        printf("read xtal cal from flash fail\r\n");
    else
        printf("read xtal cal from flash succeed:0x%02x\r\n", xtal_cal);


    /* Update from efuse */
    /* update xtal calibration from efuse,xtal calibration stored in efuse will be wrote in corresponding reg */
    ret = update_xtal_cal_from_efuse();
    if (ret < 0)
        printf("update xtal cal from efuse fail\r\n");
    else
        printf("update xtal cal from efuse succeed\r\n");

    /* update tx power phy from efuse, tx power phy g/n & b stored in efuse will be wrote in corresponding reg */
    ret = update_tx_power_from_efuse();
    if (ret < 0)
        printf("update tx power phy from flash fail\r\n");
    else
        printf("update tx power phy from flash succeed\r\n");


    /* Read Efuse, almost same with Write Flash*/
    /* read xtal calibration from efuse, value will be stored in xtal_cal*/
    ret = wland_read_xtal_cal_from_efuse((unsigned char *)&xtal_cal);
    if (ret < 0)
        printf("read xtal cal from efuse fail\r\n");
    else
        printf("read xtal cal from efuse succeed:0x%02x\r\n", xtal_cal);

    /* read tx power phy from efuse, value will be stored in array tx_power_phy */
    ret = wland_read_tx_power_from_efuse((unsigned char *)&tx_power_phy[0]);
    if (ret < 0)
        printf("read tx power phy from efuse fail\r\n");
    else
        printf("read tx power phy efuse succeed: g/n:0x%02x b:0x%02x\r\n", tx_power_phy[0], tx_power_phy[1]);

    while (true);
}

