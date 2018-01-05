#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "console.h"
#include "rda_wdt_api.h"
#include "at.h"
#include "rda5991h_wland.h"
#include "wland_rf.h"
#include "rda5981_flash.h"

#define MAC_FORMAT                   "0x%02x:0x%02x:0x%02x:0x%02x:0x%02x:0x%02x"
#define MAC2STR(ea)                  (ea)[0], (ea)[1], (ea)[2], (ea)[3], (ea)[4], (ea)[5]
#define RF_CHANNEL_FORMAT            "0x%04x 0x%04x 0x%04x 0x%04x 0x%04x 0x%04x 0x%04x 0x%04x 0x%04x 0x%04x 0x%04x 0x%04x 0x%04x 0x%04x"  
#define RF_CHANNEL2STR(ea)           (ea)[0], (ea)[1], (ea)[2], (ea)[3], (ea)[4], (ea)[5], (ea)[6], (ea)[7], (ea)[8], (ea)[9], (ea)[10], (ea)[11], (ea)[12], (ea)[13]
#define PHY_CHANNEL_FORMAT           "0x%08x 0x%08x 0x%08x 0x%08x 0x%08x 0x%08x 0x%08x 0x%08x 0x%08x 0x%08x 0x%08x 0x%08x 0x%08x 0x%08x"  
#define PHY_CHANNEL2STR(ea)          (ea)[0], (ea)[1], (ea)[2], (ea)[3], (ea)[4], (ea)[5], (ea)[6], (ea)[7], (ea)[8], (ea)[9], (ea)[10], (ea)[11], (ea)[12], (ea)[13]

typedef enum {
    TX_MODE_N = 0,
    TX_MODE_G = 1
} tx_mode_t;

static char *version = "**********RDA Software Version rdahut_V1.0**********";
extern unsigned int echo_flag;
extern int wland_dbg_dump;
extern int wland_dbg_level;
extern int wpa_dbg_dump;
extern int wpa_dbg_level;
extern int hut_dbg_dump;

static tx_mode_t last_tx_mode = TX_MODE_N;

int do_h( cmd_tbl_t *cmd, int argc, char *argv[])
{
    cmd_tbl_t *cmdtemp = NULL;
    int i;
    
    for (i = 0; i<(int)cmd_cntr; i++) {
        cmdtemp = &cmd_list[i];
        if(cmdtemp->usage) {
            printf("%s\r\n", cmdtemp->usage);
        }
    }
    return 0;
}

int do_dbg( cmd_tbl_t *cmd, int argc, char *argv[])
{
    int arg;
    
    if (argc < 3) {
        RESP_ERROR(ERROR_ARG);
        return 0;
    }

    arg = atoi(argv[2]);
    if (!strcmp(argv[1], "DRV")) {
        wland_dbg_level = arg;
    } else if (!strcmp(argv[1], "WPA")) {
        wpa_dbg_level = arg;
    } else if (!strcmp(argv[1], "DRVD")) {
        wland_dbg_dump = arg;
    } else if (!strcmp(argv[1], "WPAD")) {
        wpa_dbg_dump = arg;
    } else if (!strcmp(argv[1], "HUTD")) {
        hut_dbg_dump = arg;
    } else {
        RESP_ERROR(ERROR_ARG);
        return 0;
    }
    
    RESP_OK();

    return 0;
}

int do_ver( cmd_tbl_t *cmd, int argc, char *argv[])
{
    RESP_OK_EQU("%s\r\n", version);
    return 0;
}

int do_at( cmd_tbl_t *cmd, int argc, char *argv[])
{
    RESP_OK();
    return 0;
}

int do_rst( cmd_tbl_t *cmd, int argc, char *argv[])
{
    RESP_OK();
    rda_wdt_softreset();
    return 0;

}

int do_echo( cmd_tbl_t *cmd, int argc, char *argv[])
{
    int flag;
    
    if (argc < 2) {
        RESP_ERROR(ERROR_ARG);
        return 0;
    }
    flag = atoi(argv[1]);

    if(flag != 0 && flag != 1){
        RESP_ERROR(ERROR_ARG);
        return 0;
    }
        
    echo_flag = flag;
    RESP_OK();
    return 0;

}

int do_read_efuse(cmd_tbl_t *cmd, int argc, char *argv[])
{
    int ret = 0;
    u8 efuse[28] = {0};

    ret = wland_read_efuse(efuse);
    
    if (ret < 0) {
        RESP_ERROR(ERROR_FAILE);
        return -1;
    }

    printf("+ok=");
    for (int i = 0; i < 27; i ++) {
        printf("0x%02x ", efuse[i]);
    }
    printf("0x%02x\r\n", efuse[27]);

    return 0;
}

int do_write_tx_power_to_efuse(cmd_tbl_t *cmd, int argc, char *argv[])
{
    int ret = 0;
    u8 tx_power[2] = {0};
    
    if(argc < 3) {
        RESP_ERROR(ERROR_ARG);
        return -1;
    }

    for (int i = 1; i < 3; i++) {
        if ((0 != strncmp(argv[i], "0x", 2)) && (0 != strncmp(argv[i], "0X", 2))) {
            RESP_ERROR(ERROR_ARG);
            return -1;
        }

        tx_power[i - 1] = strtoul(argv[i], NULL, 16);
    }

    ret = wland_write_tx_power_to_efuse(tx_power, 2);

    if (ret < 0) {
        RESP_ERROR(ERROR_FAILE);
        return -1;
    }

    RESP_OK();

    return 0;
}

int do_read_tx_power_from_efuse(cmd_tbl_t *cmd, int argc, char *argv[])
{
    int ret = 0;
    u8 tx_power[2] = {0};

    ret = wland_read_tx_power_from_efuse(tx_power);

    if (ret < 0) {
        RESP_ERROR(ERROR_FAILE);
        return -1;
    }

    RESP_OK_EQU("11g/n:0x%02x 11b:0x%02x\r\n", tx_power[0], tx_power[1]);

    return 0;
}

int do_write_xtal_cal_to_efuse(cmd_tbl_t *cmd, int argc, char *argv[])
{
    int ret = 0;
    u8 xtal_cal = 0;
    
    if(argc < 2) {
        RESP_ERROR(ERROR_ARG);
        return -1;
    }

    if ((0 != strncmp(argv[1], "0x", 2)) && (0 != strncmp(argv[1], "0X", 2))) {
        RESP_ERROR(ERROR_ARG);
        return -1;
    }
    xtal_cal = strtoul(argv[1], NULL, 16);

    ret = wland_write_xtal_cal_to_efuse(&xtal_cal, 1);

    if (ret < 0) {
        RESP_ERROR(ERROR_FAILE);
        return -1;
    }

    RESP_OK();

    return 0;
}

int do_read_xtal_cal_from_efuse(cmd_tbl_t *cmd, int argc, char *argv[])
{
    int ret = 0;
    u8 xtal_cal = 0;

    ret = wland_read_xtal_cal_from_efuse(&xtal_cal);

    if (ret < 0) {
        RESP_ERROR(ERROR_FAILE);
        return -1;
    }

    RESP_OK_EQU("0x%02x\r\n", xtal_cal);

    return 0;
}

int do_write_mac_addr_to_efuse(cmd_tbl_t *cmd, int argc, char *argv[])
{
    int ret = 0;
    u8 mac_addr[6] = {0};
    
    if(argc < 7) {
        RESP_ERROR(ERROR_ARG);
        return -1;
    }

    for (int i = 1; i < 7; i++) {
        if ((0 != strncmp(argv[i], "0x", 2)) && (0 != strncmp(argv[i], "0X", 2))) {
            RESP_ERROR(ERROR_ARG);
            return -1;
        }

        mac_addr[i - 1] = strtoul(argv[i], NULL, 16);
    }

    ret = wland_write_mac_addr_to_efuse(mac_addr, 6);

    if (ret < 0) {
        RESP_ERROR(ERROR_FAILE);
        return -1;
    }

    RESP_OK();

    return 0;
}

int do_read_mac_addr_from_efuse(cmd_tbl_t *cmd, int argc, char *argv[])
{
    int ret = 0;
    u8 mac_addr[6] = {0};

    ret = wland_read_mac_addr_from_efuse(mac_addr);

    if (ret < 0) {
        RESP_ERROR(ERROR_FAILE);
        return -1;
    }

    RESP_OK_EQU(MAC_FORMAT"\r\n", MAC2STR(mac_addr));

    return 0;
}

int do_rf_write(cmd_tbl_t *cmd, int argc, char *argv[])
{
    int ret = 0;
    u16 reg;
    u16 value;
    
    if(argc < 3) {
        RESP_ERROR(ERROR_ARG);
        return -1;
    }

    for (int i = 1; i < 3; i++) {
        if ((0 != strncmp(argv[i], "0x", 2)) && (0 != strncmp(argv[i], "0X", 2))) {
            RESP_ERROR(ERROR_ARG);
            return -1;
        }

        if (1 == i)
            reg = strtoul(argv[i], NULL, 16);
        else
            value = strtoul(argv[i], NULL, 16);
    }

    ret = wland_rf_write(reg, value);

    if (ret < 0) {
        RESP_ERROR(ERROR_FAILE);
        return -1;
    }

    RESP_OK();

    return 0;
}

int do_rf_write_flash(cmd_tbl_t *cmd, int argc, char *argv[])
{
    int ret = 0;
    u16 reg;
    u16 value;

    ret = do_rf_write(cmd, argc, argv);

    if (ret < 0) {
        RESP_ERROR(ERROR_FAILE);
        return -1;
    }
    
    reg = strtoul(argv[1], NULL, 16);
    value = strtoul(argv[2], NULL, 16);

    if (0xDA == reg) {
        value &= 0xFE;
    }
    ret = rda5981_write_user_data_regs((u8 *)&reg, (u8 *)&value, RDA5991H_USER_DATA_FLAG_RF);
    
    if (ret < 0) {
        RESP_ERROR(ERROR_FAILE);
        return -1;
    }

    RESP_OK();

    return 0;
}

int do_rf_write_all_channels(cmd_tbl_t *cmd, int argc, char *argv[])
{
    int ret = 0;
    u16 reg;
    u16 value[WLAND_CHANNEL_NUM];
    
    if(argc < 16) {
        RESP_ERROR(ERROR_ARG);
        return -1;
    }

    for (int i = 1; i < 16; i++) {
        if ((0 != strncmp(argv[i], "0x", 2)) && (0 != strncmp(argv[i], "0X", 2))) {
            RESP_ERROR(ERROR_ARG);
            return -1;
        }

        if (1 == i)
            reg = strtoul(argv[i], NULL, 16);
        else
            value[i - 2] = strtoul(argv[i], NULL, 16);
    }

    ret = wland_rf_write_all_channels(reg, value, WLAND_CHANNEL_NUM);

    if (ret < 0) {
        RESP_ERROR(ERROR_FAILE);
        return -1;
    }

    RESP_OK();

    return 0;
}

int do_rf_write_all_channels_flash(cmd_tbl_t *cmd, int argc, char *argv[])
{
    int ret = 0;
    u16 reg;
    u16 value[WLAND_CHANNEL_NUM];

    ret = do_rf_write_all_channels(cmd, argc, argv);

    if (ret < 0) {
        RESP_ERROR(ERROR_FAILE);
        return -1;
    }
    
    for (int i = 1; i < 16; i++) {
        if (1 == i)
            reg = strtoul(argv[i], NULL, 16);
        else
            value[i - 2] = strtoul(argv[i], NULL, 16);
    }

    ret = rda5981_write_user_data_regs((u8 *)&reg, (u8 *)&value, RDA5991H_USER_DATA_FLAG_RF_CHANNELS);

    if (ret < 0) {
        RESP_ERROR(ERROR_FAILE);
        return -1;
    }

    RESP_OK();

    return 0;
}

int do_rf_write_single_channel(cmd_tbl_t *cmd, int argc, char *argv[])
{
    int ret = 0;
    u16 reg;
    u16 channel;
    u16 channel_value;
    u16 value[WLAND_CHANNEL_NUM];
    
    if(argc < 4) {
        RESP_ERROR(ERROR_ARG);
        return -1;
    }

    for (int i = 1; i < 4; i++) {
        if ((2 != i) && (0 != strncmp(argv[i], "0x", 2)) && (0 != strncmp(argv[i], "0X", 2))) {
            RESP_ERROR(ERROR_ARG);
            return -1;
        }

        if (1 == i)
            reg = strtoul(argv[i], NULL, 16);
        else if (2 == i)
            channel = strtoul(argv[i], NULL, 0);
        else
            channel_value = strtoul(argv[i], NULL, 16);
    }

    if ((channel < 1) || (channel > 14)) {
        RESP_ERROR(ERROR_ARG);
        return -1;
    }

    ret = wland_rf_read_all_channels(reg, value);

    if (ret < 0) {
        RESP_ERROR(ERROR_FAILE);
        return -1;
    }

    value[channel - 1] = channel_value;

    ret = wland_rf_write_all_channels(reg, value, WLAND_CHANNEL_NUM);

    if (ret < 0) {
        RESP_ERROR(ERROR_FAILE);
        return -1;
    }

    RESP_OK();

    return 0;
}

int do_rf_write_single_channel_flash(cmd_tbl_t *cmd, int argc, char *argv[])
{
    int ret = 0;
    u16 reg;
    u16 channel;
    u16 channel_value;
    u16 value[WLAND_CHANNEL_NUM];

    ret = do_rf_write_single_channel(cmd, argc, argv);

    if (ret < 0) {
        RESP_ERROR(ERROR_FAILE);
        return -1;
    }
    
    for (int i = 1; i < 4; i++) {
        if (1 == i)
            reg = strtoul(argv[i], NULL, 16);
        else if (2 == i)
            channel = strtoul(argv[i], NULL, 0);
        else
            channel_value = strtoul(argv[i], NULL, 16);
    }

    ret = wland_rf_read_all_channels(reg, value);

    if (ret < 0) {
        RESP_ERROR(ERROR_FAILE);
        return -1;
    }

    value[channel -1] = channel_value;

    ret = rda5981_write_user_data_regs((u8 *)&reg, (u8 *)&value, RDA5991H_USER_DATA_FLAG_RF_CHANNELS);

    if (ret < 0) {
        RESP_ERROR(ERROR_FAILE);
        return -1;
    }

    RESP_OK();

    return 0;
}

int do_rf_read(cmd_tbl_t *cmd, int argc, char *argv[])
{
    int ret = 0;
    u16 reg;
    u16 value;

    if (argc < 2) {
        RESP_ERROR(ERROR_ARG);
        return -1;
    }

    if ((0 != strncmp(argv[1], "0x", 2)) && (0 != strncmp(argv[1], "0X", 2))) {
        RESP_ERROR(ERROR_ARG);
        return -1;
    }
    reg = strtoul(argv[1], NULL, 16);

    ret = wland_rf_read(reg, &value);

    if (ret < 0) {
        RESP_ERROR(ERROR_FAILE);
        return -1;
    }

    RESP_OK_EQU("0x%04x\r\n", value);

    return 0;
}

int do_rf_read_flash(cmd_tbl_t *cmd, int argc, char *argv[])
{
    int ret = 0;
    u16 reg;
    u16 value;

    if (argc < 2) {
        RESP_ERROR(ERROR_ARG);
        return -1;
    }

    if ((0 != strncmp(argv[1], "0x", 2)) && (0 != strncmp(argv[1], "0X", 2))) {
        RESP_ERROR(ERROR_ARG);
        return -1;
    }
    reg = strtoul(argv[1], NULL, 16);

    ret = rda5981_read_user_data_regs((u8 *)&reg, (u8 *)&value, RDA5991H_USER_DATA_FLAG_RF);

    if (ret < 0) {
        RESP_ERROR(ERROR_FAILE);
        return -1;
    }

    RESP_OK_EQU("0x%04x\r\n", value);

    return 0;
}

int do_rf_read_all_channels(cmd_tbl_t *cmd, int argc, char *argv[])
{
    int ret = 0;
    u16 reg;
    u16 value[WLAND_CHANNEL_NUM];

    if (argc < 2) {
        RESP_ERROR(ERROR_ARG);
        return -1;
    }

    if ((0 != strncmp(argv[1], "0x", 2)) && (0 != strncmp(argv[1], "0X", 2))) {
        RESP_ERROR(ERROR_ARG);
        return -1;
    }
    reg = strtoul(argv[1], NULL, 16);

    ret = wland_rf_read_all_channels(reg, value);

    if (ret < 0) {
        RESP_ERROR(ERROR_FAILE);
        return -1;
    }

    RESP_OK_EQU(RF_CHANNEL_FORMAT"\r\n", RF_CHANNEL2STR(value));
    
    return 0;
}

int do_rf_read_all_channels_flash(cmd_tbl_t *cmd, int argc, char *argv[])
{
    int ret = 0;
    u16 reg;
    u16 value[WLAND_CHANNEL_NUM];

    if (argc < 2) {
        RESP_ERROR(ERROR_ARG);
        return -1;
    }

    if ((0 != strncmp(argv[1], "0x", 2)) && (0 != strncmp(argv[1], "0X", 2))) {
        RESP_ERROR(ERROR_ARG);
        return -1;
    }
    reg = strtoul(argv[1], NULL, 16);

    ret = rda5981_read_user_data_regs((u8 *)&reg, (u8 *)&value, RDA5991H_USER_DATA_FLAG_RF_CHANNELS);
    
    if (ret < 0) {
        RESP_ERROR(ERROR_FAILE);
        return -1;
    }

    RESP_OK_EQU(RF_CHANNEL_FORMAT"\r\n", RF_CHANNEL2STR(value));
    
    return 0;
}

int do_rf_read_single_channel(cmd_tbl_t *cmd, int argc, char *argv[])
{
    int ret = 0;
    u16 reg;
    u16 channel;
    u16 value[WLAND_CHANNEL_NUM];
    
    if(argc < 3) {
        RESP_ERROR(ERROR_ARG);
        return -1;
    }

    if ((0 != strncmp(argv[1], "0x", 2)) && (0 != strncmp(argv[1], "0X", 2))) {
        RESP_ERROR(ERROR_ARG);
        return -1;
    }
    reg = strtoul(argv[1], NULL, 16);
    
    channel = strtoul(argv[2], NULL, 0);

    if ((channel < 1) || (channel > 14)) {
        RESP_ERROR(ERROR_ARG);
        return -1;
    }

    ret = wland_rf_read_all_channels(reg, value);

    if (ret < 0) {
        RESP_ERROR(ERROR_FAILE);
        return -1;
    }

    RESP_OK_EQU("0x%04x\r\n", value[channel - 1]);

    return 0;
}

int do_rf_read_single_channel_flash(cmd_tbl_t *cmd, int argc, char *argv[])
{
    int ret = 0;
    u16 reg;
    u16 channel;
    u16 value[WLAND_CHANNEL_NUM];
    
    if(argc < 3) {
        RESP_ERROR(ERROR_ARG);
        return -1;
    }

    if ((0 != strncmp(argv[1], "0x", 2)) && (0 != strncmp(argv[1], "0X", 2))) {
        RESP_ERROR(ERROR_ARG);
        return -1;
    }
    reg = strtoul(argv[1], NULL, 16);
    
    channel = strtoul(argv[2], NULL, 0);

    if ((channel < 1) || (channel > 14)) {
        RESP_ERROR(ERROR_ARG);
        return -1;
    }

    ret = rda5981_read_user_data_regs((u8 *)&reg, (u8 *)&value, RDA5991H_USER_DATA_FLAG_RF_CHANNELS);

    if (ret < 0) {
        RESP_ERROR(ERROR_FAILE);
        return -1;
    }

    RESP_OK_EQU("0x%04x\r\n", value[channel - 1]);

    return 0;
}

int do_phy_write(cmd_tbl_t *cmd, int argc, char *argv[])
{
    int ret = 0;
    u32 reg;
    u32 value;
    
    if(argc < 3) {
        RESP_ERROR(ERROR_ARG);
        return -1;
    }

    for (int i = 1; i < 3; i++) {
        if ((0 != strncmp(argv[i], "0x", 2)) && (0 != strncmp(argv[i], "0X", 2))) {
            RESP_ERROR(ERROR_ARG);
            return -1;
        }

        if (1 == i)
            reg = strtoul(argv[i], NULL, 16);
        else
            value = strtoul(argv[i], NULL, 16);
    }

    ret = wland_phy_write(reg, value);

    if (ret < 0) {
        RESP_ERROR(ERROR_FAILE);
        return -1;
    }

    RESP_OK();

    return 0;
}

int do_phy_write_flash(cmd_tbl_t *cmd, int argc, char *argv[])
{
    int ret = 0;
    u32 reg;
    u32 value;

    ret = do_phy_write(cmd, argc, argv);

    if (ret < 0) {
        RESP_ERROR(ERROR_FAILE);
        return -1;
    }
    
    reg = strtoul(argv[1], NULL, 16);
    value = strtoul(argv[2], NULL, 16);
    
    ret = rda5981_write_user_data_regs((u8 *)&reg, (u8 *)&value, RDA5991H_USER_DATA_FLAG_PHY);
    
    if (ret < 0) {
        RESP_ERROR(ERROR_FAILE);
        return -1;
    }

    RESP_OK();

    return 0;
}


int do_phy_write_all_channels(cmd_tbl_t *cmd, int argc, char *argv[])
{
    int ret = 0;
    u32 reg;
    u32 value[WLAND_CHANNEL_NUM];
    
    if(argc < 16) {
        RESP_ERROR(ERROR_ARG);
        return -1;
    }

    for (int i = 1; i < 16; i++) {
        if ((0 != strncmp(argv[i], "0x", 2)) && (0 != strncmp(argv[i], "0X", 2))) {
            RESP_ERROR(ERROR_ARG);
            return -1;
        }

        if (1 == i)
            reg = strtoul(argv[i], NULL, 16);
        else
            value[i - 2] = strtoul(argv[i], NULL, 16);
    }

    ret = wland_phy_write_all_channels(reg, value, WLAND_CHANNEL_NUM);

    if (ret < 0) {
        RESP_ERROR(ERROR_FAILE);
        return -1;
    }

    RESP_OK();

    return 0;
}

int do_phy_write_all_channels_flash(cmd_tbl_t *cmd, int argc, char *argv[])
{
    int ret = 0;
    u32 reg;
    u32 value[WLAND_CHANNEL_NUM];

    ret = do_phy_write_all_channels(cmd, argc, argv);

    if (ret < 0) {
        RESP_ERROR(ERROR_FAILE);
        return -1;
    }
    
    for (int i = 1; i < 16; i++) {
        if (1 == i)
            reg = strtoul(argv[i], NULL, 16);
        else
            value[i - 2] = strtoul(argv[i], NULL, 16);
    }

    ret = rda5981_write_user_data_regs((u8 *)&reg, (u8 *)&value, RDA5991H_USER_DATA_FLAG_PHY_CHANNELS);

    if (ret < 0) {
        RESP_ERROR(ERROR_FAILE);
        return -1;
    }

    RESP_OK();

    return 0;
}

int do_phy_write_single_channel(cmd_tbl_t *cmd, int argc, char *argv[])
{
    int ret = 0;
    u32 reg;
    u32 channel;
    u32 channel_value;
    u32 value[WLAND_CHANNEL_NUM];
    
    if(argc < 4) {
        RESP_ERROR(ERROR_ARG);
        return -1;
    }

    for (int i = 1; i < 4; i++) {
        if ((2 != i) && (0 != strncmp(argv[i], "0x", 2)) && (0 != strncmp(argv[i], "0X", 2))) {
            RESP_ERROR(ERROR_ARG);
            return -1;
        }

        if (1 == i)
            reg = strtoul(argv[i], NULL, 16);
        else if (2 == i)
            channel = strtoul(argv[i], NULL, 0);
        else
            channel_value = strtoul(argv[i], NULL, 16);
    }

    if ((channel < 1) || (channel > 14)) {
        RESP_ERROR(ERROR_ARG);
        return -1;
    }

    ret = wland_phy_read_all_channels(reg, value);

    if (ret < 0) {
        RESP_ERROR(ERROR_FAILE);
        return -1;
    }

    value[channel - 1] = channel_value;

    ret = wland_phy_write_all_channels(reg, value, WLAND_CHANNEL_NUM);

    if (ret < 0) {
        RESP_ERROR(ERROR_FAILE);
        return -1;
    }

    RESP_OK();

    return 0;
}

int do_phy_write_single_channel_flash(cmd_tbl_t *cmd, int argc, char *argv[])
{
    int ret = 0;
    u32 reg;
    u32 channel;
    u32 channel_value;
    u32 value[WLAND_CHANNEL_NUM];

    ret = do_phy_write_single_channel(cmd, argc, argv);

    if (ret < 0) {
        RESP_ERROR(ERROR_FAILE);
        return -1;
    }
    
    for (int i = 1; i < 4; i++) {
        if (1 == i)
            reg = strtoul(argv[i], NULL, 16);
        else if (2 == i)
            channel = strtoul(argv[i], NULL, 0);
        else
            channel_value = strtoul(argv[i], NULL, 16);
    }

    ret = wland_phy_read_all_channels(reg, value);

    if (ret < 0) {
        RESP_ERROR(ERROR_FAILE);
        return -1;
    }

    value[channel -1] = channel_value;

    ret = rda5981_write_user_data_regs((u8 *)&reg, (u8 *)&value, RDA5991H_USER_DATA_FLAG_PHY_CHANNELS);

    if (ret < 0) {
        RESP_ERROR(ERROR_FAILE);
        return -1;
    }

    RESP_OK();

    return 0;
}

int do_phy_read(cmd_tbl_t *cmd, int argc, char *argv[])
{
    int ret = 0;
    u32 reg;
    u32 value;

    if (argc < 2) {
        RESP_ERROR(ERROR_ARG);
        return -1;
    }

    if ((0 != strncmp(argv[1], "0x", 2)) && (0 != strncmp(argv[1], "0X", 2))) {
        RESP_ERROR(ERROR_ARG);
        return -1;
    }
    reg = strtoul(argv[1], NULL, 16);

    ret = wland_phy_read(reg, &value);

    if (ret < 0) {
        RESP_ERROR(ERROR_FAILE);
        return -1;
    }

    RESP_OK_EQU("0x%08x\r\n", value);

    return 0;
}

int do_phy_read_flash(cmd_tbl_t *cmd, int argc, char *argv[])
{
    int ret = 0;
    u32 reg;
    u32 value;

    if (argc < 2) {
        RESP_ERROR(ERROR_ARG);
        return -1;
    }

    if ((0 != strncmp(argv[1], "0x", 2)) && (0 != strncmp(argv[1], "0X", 2))) {
        RESP_ERROR(ERROR_ARG);
        return -1;
    }
    reg = strtoul(argv[1], NULL, 16);

    ret = rda5981_read_user_data_regs((u8 *)&reg, (u8 *)&value, RDA5991H_USER_DATA_FLAG_PHY);

    if (ret < 0) {
        RESP_ERROR(ERROR_FAILE);
        return -1;
    }

    RESP_OK_EQU("0x%08x\r\n", value);

    return 0;
}


int do_phy_read_all_channels(cmd_tbl_t *cmd, int argc, char *argv[])
{
    int ret = 0;
    u32 reg;
    u32 value[WLAND_CHANNEL_NUM];

    if (argc < 2) {
        RESP_ERROR(ERROR_ARG);
        return -1;
    }

    if ((0 != strncmp(argv[1], "0x", 2)) && (0 != strncmp(argv[1], "0X", 2))) {
        RESP_ERROR(ERROR_ARG);
        return -1;
    }
    reg = strtoul(argv[1], NULL, 16);

    ret = wland_phy_read_all_channels(reg, value);

    if (ret < 0) {
        RESP_ERROR(ERROR_FAILE);
        return -1;
    }

    RESP_OK_EQU(PHY_CHANNEL_FORMAT"\r\n", PHY_CHANNEL2STR(value));

    return 0;
}

int do_phy_read_all_channels_flash(cmd_tbl_t *cmd, int argc, char *argv[])
{
    int ret = 0;
    u32 reg;
    u32 value[WLAND_CHANNEL_NUM];

    if (argc < 2) {
        RESP_ERROR(ERROR_ARG);
        return -1;
    }

    if ((0 != strncmp(argv[1], "0x", 2)) && (0 != strncmp(argv[1], "0X", 2))) {
        RESP_ERROR(ERROR_ARG);
        return -1;
    }
    reg = strtoul(argv[1], NULL, 16);

    ret = rda5981_read_user_data_regs((u8 *)&reg, (u8 *)&value, RDA5991H_USER_DATA_FLAG_PHY_CHANNELS);
    
    if (ret < 0) {
        RESP_ERROR(ERROR_FAILE);
        return -1;
    }

    RESP_OK_EQU(PHY_CHANNEL_FORMAT"\r\n", PHY_CHANNEL2STR(value));
    
    return 0;
}

int do_phy_read_single_channel(cmd_tbl_t *cmd, int argc, char *argv[])
{
    int ret = 0;
    u32 reg;
    u32 channel;
    u32 value[WLAND_CHANNEL_NUM];
    
    if(argc < 3) {
        RESP_ERROR(ERROR_ARG);
        return -1;
    }

    if ((0 != strncmp(argv[1], "0x", 2)) && (0 != strncmp(argv[1], "0X", 2))) {
        RESP_ERROR(ERROR_ARG);
        return -1;
    }
    reg = strtoul(argv[1], NULL, 16);
    
    channel = strtoul(argv[2], NULL, 0);

    if ((channel < 1) || (channel > 14)) {
        RESP_ERROR(ERROR_ARG);
        return -1;
    }

    ret = wland_phy_read_all_channels(reg, value);

    if (ret < 0) {
        RESP_ERROR(ERROR_FAILE);
        return -1;
    }

    RESP_OK_EQU("0x%08x\r\n", value[channel - 1]);

    return 0;
}

int do_phy_read_single_channel_flash(cmd_tbl_t *cmd, int argc, char *argv[])
{
    int ret = 0;
    u32 reg;
    u32 channel;
    u32 value[WLAND_CHANNEL_NUM];
    
    if(argc < 3) {
        RESP_ERROR(ERROR_ARG);
        return -1;
    }

    if ((0 != strncmp(argv[1], "0x", 2)) && (0 != strncmp(argv[1], "0X", 2))) {
        RESP_ERROR(ERROR_ARG);
        return -1;
    }
    reg = strtoul(argv[1], NULL, 16);
    
    channel = strtoul(argv[2], NULL, 0);

    if ((channel < 1) || (channel > 14)) {
        RESP_ERROR(ERROR_ARG);
        return -1;
    }

    ret = rda5981_read_user_data_regs((u8 *)&reg, (u8 *)&value, RDA5991H_USER_DATA_FLAG_PHY_CHANNELS);

    if (ret < 0) {
        RESP_ERROR(ERROR_FAILE);
        return -1;
    }

    RESP_OK_EQU("0x%08x\r\n", value[channel - 1]);

    return 0;
}

int do_write_mac_flash(cmd_tbl_t *cmd, int argc, char *argv[])
{
    int ret = 0;
    u8 mac[6];

    if(argc < 7) {
        RESP_ERROR(ERROR_ARG);
        return -1;
    }

    for (int i = 1; i < 7; i++) {
        if ((0 != strncmp(argv[i], "0x", 2)) && (0 != strncmp(argv[i], "0X", 2))) {
            RESP_ERROR(ERROR_ARG);
            return -1;
        }
        mac[i - 1] = strtoul(argv[i], NULL, 16);
    }
    
    ret = rda5981_flash_write_mac_addr(mac);
    
    if (ret < 0) {
        RESP_ERROR(ERROR_FAILE);
        return -1;
    }

    RESP_OK();

    return 0;
}

int do_read_mac_flash(cmd_tbl_t *cmd, int argc, char *argv[])
{
    int ret = 0;
    u8 mac[6];
    
    ret = rda5981_flash_read_mac_addr(mac);
    
    if (ret < 0) {
        RESP_ERROR(ERROR_FAILE);
        return -1;
    }

    RESP_OK_EQU(MAC_FORMAT"\r\n", MAC2STR(mac));

    return 0;
}

int do_start_tx_test(cmd_tbl_t *cmd, int argc, char *argv[])
{
    int ret = 0;
    u32 value[6];
    u32 tp_reg = 0x11F;
    u32 tp_value = 0;
    u8 tp_offset = 0;

    if(argc < 7) {
        RESP_ERROR(ERROR_ARG);
        return -1;
    }

    for (int i = 1; i < 7; i++) {
        value[i - 1] = strtoul(argv[i], NULL, 0);
    }

    ret = wland_phy_read(tp_reg, &tp_value);
    if (ret < 0) {
        RESP_ERROR(ERROR_FAILE);
        return -1;
    }

    ret = rda5981_read_user_data(&tp_offset, 1, RDA5991H_USER_DATA_FLAG_TX_POWER_OFFSET);
    if (ret < 0) {
        tp_offset = 0;
    }

    if (tp_offset) {
        if ((value[1] == 0) && ((value[4] == 6) || (value[4] == 9) || (value[4] == 12) || (value[4] == 18) ||
            (value[4] == 24) || (value[4] == 36) || (value[4] == 48) || (value[4] ==54))) {//G Mode
            if (TX_MODE_N == last_tx_mode) {
                tp_value += tp_offset;
                ret = wland_phy_write(tp_reg, tp_value);
                last_tx_mode = TX_MODE_G;
            }
        } else if (((value[1] == 1) || (value[1] == 2)) && (value[4] <= 7)) {//N Mode
            if (TX_MODE_G == last_tx_mode) {
                tp_value -= tp_offset;
                ret = wland_phy_write(tp_reg, tp_value);
                last_tx_mode = TX_MODE_N;
            }
        }
    }

    ret = wland_start_rf_test(6, value, 1);

    if (ret < 0) {
        RESP_ERROR(ERROR_FAILE);
        return -1;
    }

#if defined (HI_FLYING)
    ret = wland_rf_write(0x1A, 0x02);
    if (ret < 0) {
        RESP_ERROR(ERROR_FAILE);
        return -1;
    }
#endif

    RESP_OK();

    return 0;
}

int do_stop_tx_test(cmd_tbl_t *cmd, int argc, char *argv[])
{
    int ret = 0;

    ret = wland_stop_tx_test();

    if (ret < 0) {
        RESP_ERROR(ERROR_FAILE);
        return -1;
    }

    RESP_OK();

    return 0;
}

int do_restart_tx_test(cmd_tbl_t *cmd, int argc, char *argv[])
{
    int ret = 0;

    ret = wland_restart_tx_test();

    if (ret < 0) {
        RESP_ERROR(ERROR_FAILE);
        return -1;
    }

    RESP_OK();

    return 0;
}

int do_start_rx_test(cmd_tbl_t *cmd, int argc, char *argv[])
{
    int ret = 0;
    u32 value[4];

    if(argc < 5) {
        RESP_ERROR(ERROR_ARG);
        return -1;
    }

    for (int i = 1; i < 5; i++) {
        value[i - 1] = strtoul(argv[i], NULL, 0);
    }

    ret = wland_start_rf_test(4, value, 0);

    if (ret < 0) {
        RESP_ERROR(ERROR_FAILE);
        return -1;
    }

    RESP_OK();

    return 0;
}

int do_stop_rx_test(cmd_tbl_t *cmd, int argc, char *argv[])
{
    int ret = 0;
    
    ret = wland_stop_rx_test();

    if (ret < 0) {
        RESP_ERROR(ERROR_FAILE);
        return -1;
    }

    RESP_OK();

    return 0;
}

int do_get_rx_result(cmd_tbl_t *cmd, int argc, char *argv[])
{
    int ret = 0;
    char result[120];

    ret = wland_get_rx_result(result);

    if (ret < 0) {
        RESP_ERROR(ERROR_FAILE);
        return -1;
    }

    RESP_OK_EQU("%s\r\n", result);

    return 0;
}

int do_restart_rx_test(cmd_tbl_t *cmd, int argc, char *argv[])
{
    int ret = 0;

    ret = wland_restart_rx_test();

    if (ret < 0) {
        RESP_ERROR(ERROR_FAILE);
        return -1;
    }

    RESP_OK();

    return 0;
}

int do_dump_rf_flash(cmd_tbl_t *cmd, int argc, char *argv[])
{
    int ret = 0;
    wland_rf_t rf;

    ret = rda5981_read_user_data((u8 *)&rf, sizeof(wland_rf_t), RDA5991H_USER_DATA_FLAG_RF);

    if (ret < 0) {
        RESP_ERROR(ERROR_FAILE);
        return -1;
    }

    printf("valid:0x%08x\n", rf.valid);
    printf("flag:0x%08x\n", rf.flag);
    for(int i = 0; i < 8; i++) {
        printf("reg:0x%04x value:0x%04x\n", rf.reg_val[i][0], rf.reg_val[i][1]);
    }

    RESP_OK();

    return 0;
}

int do_dump_rf_all_channels_flash(cmd_tbl_t *cmd, int argc, char *argv[])
{
    int ret = 0;
    wland_rf_channels_t rf_channels;

    ret = rda5981_read_user_data((u8 *)&rf_channels, sizeof(wland_rf_channels_t), RDA5991H_USER_DATA_FLAG_RF_CHANNELS);

    if (ret < 0) {
        RESP_ERROR(ERROR_FAILE);
        return -1;
    }

    printf("valid:0x%08x\n", rf_channels.valid);
    printf("flag:0x%08x\n", rf_channels.flag);
    for(int i = 0; i < 8; i++) {
        printf("reg:0x%04x value:", rf_channels.reg_val[i][0]);
        for (int j = 1; j < 15; j++) {
            printf("0x%04x ", rf_channels.reg_val[i][j]);
        }
        printf("\n");
    }

    RESP_OK();

    return 0;
}


int do_erase_rf_flash(cmd_tbl_t *cmd, int argc, char *argv[])
{
    int ret = 0;
    u16 reg;

    if(argc < 2) {
        RESP_ERROR(ERROR_ARG);
        return -1;
    }

    if ((0 != strncmp(argv[1], "0x", 2)) && (0 != strncmp(argv[1], "0X", 2))) {
        RESP_ERROR(ERROR_ARG);
        return -1;
    }
    reg = strtoul(argv[1], NULL, 16);

    ret = rda5981_erase_user_data_regs((u8 *)&reg, RDA5991H_USER_DATA_FLAG_RF);

    if (ret < 0) {
        RESP_ERROR(ERROR_FAILE);
        return -1;
    }

    RESP_OK();

    return 0;
}

int do_erase_rf_all_channels_flash(cmd_tbl_t *cmd, int argc, char *argv[])
{
    int ret = 0;
    u16 reg;

    if(argc < 2) {
        RESP_ERROR(ERROR_ARG);
        return -1;
    }

    if ((0 != strncmp(argv[1], "0x", 2)) && (0 != strncmp(argv[1], "0X", 2))) {
        RESP_ERROR(ERROR_ARG);
        return -1;
    }
    reg = strtoul(argv[1], NULL, 16);

    ret = rda5981_erase_user_data_regs((u8 *)&reg, RDA5991H_USER_DATA_FLAG_RF_CHANNELS);

    if (ret < 0) {
        RESP_ERROR(ERROR_FAILE);
        return -1;
    }

    RESP_OK();

    return 0;
}

int do_dump_phy_flash(cmd_tbl_t *cmd, int argc, char *argv[])
{
    int ret = 0;
    wland_phy_t phy;

    ret = rda5981_read_user_data((u8 *)&phy, sizeof(wland_phy_t), RDA5991H_USER_DATA_FLAG_PHY);

    if (ret < 0) {
        RESP_ERROR(ERROR_FAILE);
        return -1;
    }

    printf("valid:0x%08x\n", phy.valid);
    printf("flag:0x%08x\n", phy.flag);
    for(int i = 0; i < 8; i++) {
        printf("reg:0x%08x value:0x%08x\n", phy.reg_val[i][0], phy.reg_val[i][1]);
    }

    RESP_OK();

    return 0;
}

int do_dump_phy_all_channels_flash(cmd_tbl_t *cmd, int argc, char *argv[])
{
    int ret = 0;
    wland_phy_channels_t phy_channels;

    ret = rda5981_read_user_data((u8 *)&phy_channels, sizeof(wland_phy_channels_t), RDA5991H_USER_DATA_FLAG_PHY_CHANNELS);

    if (ret < 0) {
        RESP_ERROR(ERROR_FAILE);
        return -1;
    }

    printf("valid:0x%08x\n", phy_channels.valid);
    printf("flag:0x%08x\n", phy_channels.flag);
    for(int i = 0; i < 8; i++) {
        printf("reg:0x%08x value:", phy_channels.reg_val[i][0]);
        for (int j = 1; j < 15; j++) {
            printf("0x%08x ", phy_channels.reg_val[i][j]);
        }
        printf("\n");
    }

    RESP_OK();

    return 0;
}


int do_erase_phy_flash(cmd_tbl_t *cmd, int argc, char *argv[])
{
    int ret = 0;
    u32 reg;

    if(argc < 2) {
        RESP_ERROR(ERROR_ARG);
        return -1;
    }

    if ((0 != strncmp(argv[1], "0x", 2)) && (0 != strncmp(argv[1], "0X", 2))) {
        RESP_ERROR(ERROR_ARG);
        return -1;
    }
    reg = strtoul(argv[1], NULL, 16);

    ret = rda5981_erase_user_data_regs((u8 *)&reg, RDA5991H_USER_DATA_FLAG_PHY);

    if (ret < 0) {
        RESP_ERROR(ERROR_FAILE);
        return -1;
    }

    RESP_OK();

    return 0;
}

int do_erase_phy_all_channels_flash(cmd_tbl_t *cmd, int argc, char *argv[])
{
    int ret = 0;
    u32 reg;

    if(argc < 2) {
        RESP_ERROR(ERROR_ARG);
        return -1;
    }

    if ((0 != strncmp(argv[1], "0x", 2)) && (0 != strncmp(argv[1], "0X", 2))) {
        RESP_ERROR(ERROR_ARG);
        return -1;
    }
    reg = strtoul(argv[1], NULL, 16);

    ret = rda5981_erase_user_data_regs((u8 *)&reg, RDA5991H_USER_DATA_FLAG_PHY_CHANNELS);

    if (ret < 0) {
        RESP_ERROR(ERROR_FAILE);
        return -1;
    }

    RESP_OK();

    return 0;
}

int do_write_tx_power_offset_flash(cmd_tbl_t *cmd, int argc, char *argv[])
{
    int ret = 0;
    u8 offset;

    if(argc < 2) {
        RESP_ERROR(ERROR_ARG);
        return -1;
    }

    offset = strtoul(argv[1], NULL, 0);
    if (offset > 16) {
        RESP_ERROR(ERROR_ARG);
        return -1;
    }

    ret = rda5981_write_user_data(&offset, 1, RDA5991H_USER_DATA_FLAG_TX_POWER_OFFSET);

    if (ret < 0) {
        RESP_ERROR(ERROR_FAILE);
        return -1;
    }
    
    RESP_OK();

    return 0;
}

int do_read_tx_power_offset_flash(cmd_tbl_t *cmd, int argc, char *argv[])
{
    int ret = 0;
    u8 tp_offset;

    ret = rda5981_read_user_data(&tp_offset, 1, RDA5991H_USER_DATA_FLAG_TX_POWER_OFFSET);

    if (ret < 0) {
        RESP_ERROR(ERROR_FAILE);
        return -1;
    }
    
    RESP_OK_EQU("%d\r\n", tp_offset);

    return 0;
}

void add_cmd()
{
    int i, j;
    cmd_tbl_t cmd_list[] = {
        /*Basic CMD*/
        {
            "AT+H",             1,   do_h,
            "AT+H               - check AT help"
        },      
        {
            "AT+ECHO",          2,   do_echo,
            "AT+ECHO            - open/close uart echo"
        },
        {
            "AT+RST",           1,   do_rst,
            "AT+RST             - Software Reset"
        },
        {
            "AT+VER",           1,   do_ver,
            "AT+VER             - get version"
        },
        {
            "AT+WDBG",          3,   do_dbg,
            "AT+WDBG            - adjust debug level"
        },
        
        /* Efuse CMD*/
        {
            "AT+RE",            1,   do_read_efuse,
            "AT+RE              - read all efuse"
        },
        {
            "AT+WTPE",          3,   do_write_tx_power_to_efuse,
            "AT+WTPE            - write tx power to efuse"
        },
        {
            "AT+RTPE",          1,   do_read_tx_power_from_efuse,
            "AT+RTPE            - read tx power from efuse"
        },
        {
            "AT+WXCE",          2,   do_write_xtal_cal_to_efuse,
            "AT+WXCE            - write xtal cal to efuse"
        },
        {
            "AT+RXCE",          1,   do_read_xtal_cal_from_efuse,
            "AT+RXCE            - read xtal cal from efuse"
        },
        {
            "AT+WMACE",         7,   do_write_mac_addr_to_efuse,
            "AT+WMACE           - write mac addr to efuse"
        },
        {
            "AT+RMACE",         1,   do_read_mac_addr_from_efuse,
            "AT+RMACE           - read mac addr from efuse"
        },
        
        /* RF reg CMD */
        {
            "AT+WRF_CUR",       3,   do_rf_write,
            "AT+WRF_CUR         - write rf reg, irrelated with channel"
        },
        {
            "AT+WRF_DEF",       3,   do_rf_write_flash,
            "AT+WRF_DEF         - write rf reg, irrelated with channel, write to flash meanwhile"
        },
        {
            "AT+WRFAC_CUR",     16,  do_rf_write_all_channels,
            "AT+WRFAC_CUR       - write rf reg, all channels"
        },
        {
            "AT+WRFAC_DEF",     16,  do_rf_write_all_channels_flash,
            "AT+WRFAC_DEF       - write rf reg, all channels, write to flash meanwhile"
        },
        {
            "AT+WRFSC_CUR",     4,   do_rf_write_single_channel,
            "AT+WRFSC_CUR       - write rf reg, single channel"
        },
        {
            "AT+WRFSC_DEF",     4,   do_rf_write_single_channel_flash,
            "AT+WRFSC_DEF       - write rf reg, single channel, write to flash meanwhile"
        },
        {
            "AT+RRF_CUR",       2,   do_rf_read,
            "AT+RRF_CUR         - read rf reg, irrelated with channel"
        },
        {
            "AT+RRF_DEF",       2,   do_rf_read_flash,
            "AT+RRF_DEF         - read rf reg, irrelated with channel, from flash"
        },
        {
            "AT+RRFAC_CUR",     2,   do_rf_read_all_channels,
            "AT+RRFAC_CUR       - read rf reg, all channels"
        },
        {
            "AT+RRFAC_DEF",     2,   do_rf_read_all_channels_flash,
            "AT+RRFAC_DEF       - read rf reg, all channels, from flash"
        },
        {
            "AT+RRFSC_CUR",     3,   do_rf_read_single_channel,
            "AT+RRFSC_CUR       - read rf reg, single channel"
        },
        {
            "AT+RRFSC_DEF",     3,   do_rf_read_single_channel_flash,
            "AT+RRFSC_DEF       - read rf reg, single channel, from flash"
        },
        {
            "AT+DRF",           1,   do_dump_rf_flash,
            "AT+DRF             - dump rf reg and value stored in flash, irrelated with channels"
        },
        {
            "AT+DRFAC",         1,   do_dump_rf_all_channels_flash,
            "AT+DRFAC           - dump rf reg and value stored in flash, all channels"
        },
        {
            "AT+ERF",           2,   do_erase_rf_flash,
            "AT+ERF             - erase rf reg and value stored in flash, irrelated with channels"
        },
        {
            "AT+ERFAC",         2,   do_erase_rf_all_channels_flash,
            "AT+ERFAC           - erase rf reg and value stored in flash, all channels"
        },
        
        /* PHY reg CMD */
        {
            "AT+WPHY_CUR",      3,   do_phy_write,
            "AT+WPHY_CUR        - write phy reg, irrelated with channel"
        },
        {
            "AT+WPHY_DEF",      3,   do_phy_write_flash,
            "AT+WPHY_DEF        - write phy reg, irrelated with channel, write to flash meanwhile"
        },
        {
            "AT+WPHYAC_CUR",    16,  do_phy_write_all_channels,
            "AT+WPHYAC_CUR      - write phy reg, all channels"
        },
        {
            "AT+WPHYAC_DEF",    16,  do_phy_write_all_channels_flash,
            "AT+WPHYAC_DEF      - write phy reg, all channels, write to flash meanwhile"
        },
        {
            "AT+WPHYSC_CUR",    4,   do_phy_write_single_channel,
            "AT+WPHYSC_CUR      - write phy reg, single channel"
        },
        {
            "AT+WPHYSC_DEF",    4,   do_phy_write_single_channel_flash,
            "AT+WPHYSC_DEF      - write phy reg, single channel, write to flash meanwhile"
        },
        {
            "AT+RPHY_CUR",      2,   do_phy_read,
            "AT+RPHY_CUR        - read phy reg, irrelated with channel"
        },
        {
            "AT+RPHY_DEF",      2,   do_phy_read_flash,
            "AT+RPHY_DEF        - read phy reg, irrelated with channel, from flash"
        },
        {
            "AT+RPHYAC_CUR",    2,   do_phy_read_all_channels,
            "AT+RPHYAC_CUR      - read phy reg, all channels"
        },
        {
            "AT+RPHYAC_DEF",    2,   do_phy_read_all_channels_flash,
            "AT+RPHYAC_DEF      - read phy reg, all channels, from flash"
        },
        {
            "AT+RPHYSC_CUR",    3,   do_phy_read_single_channel,
            "AT+RPHYSC_CUR      - read phy reg, single channel"
        },
        {
            "AT+RPHYSC_DEF",    3,   do_phy_read_single_channel_flash,
            "AT+RPHYSC_DEF      - read phy reg, single channel, from flash"
        },
        {
            "AT+DPHY",          1,   do_dump_phy_flash,
            "AT+DPHY            - dump phy reg and value stored in flash, irrelated with channels"
        },
        {
            "AT+DPHYAC",        1,   do_dump_phy_all_channels_flash,
            "AT+DPHYAC          - dump phy reg and value stored in flash, all channels"
        },
        {
            "AT+EPHY",          2,   do_erase_phy_flash,
            "AT+EPHY            - erase phy reg and value stored in flash, irrelated with channels"
        },
        {
            "AT+EPHYAC",        2,   do_erase_phy_all_channels_flash,
            "AT+EPHYAC          - erase phy reg and value stored in flash, all channels"
        },

        /* MAC CMD*/
        {
            "AT+WMAC_DEF",      7,   do_write_mac_flash,
            "AT+WMAC_DEF        - write xtal cal, write to flash meanwhile"
        },
        {
            "AT+RMAC_DEF",      1,   do_read_mac_flash,
            "AT+RMAC_DEF        - read mac addr from flash"
        },
        
        /* TX/RX test CMD */
        {
            "AT+TXSTART",       7,   do_start_tx_test,
            "AT+TXSTART         - start tx test"
        },
        {
            "AT+TXSTOP",        1,   do_stop_tx_test,
            "AT+TXSTOP          - stop tx test"
        },
        {
            "AT+TXRESTART",     1,   do_restart_tx_test,
            "AT+TXRESTART       - restart tx test"
        },
        {
            "AT+RXSTART",       5,   do_start_rx_test,
            "AT+RXSTART         - start rx test"
        },
        {
            "AT+RXSTOP",        1,   do_stop_rx_test,
            "AT+RXSTOP          - stop rx test"
        },
        {
            "AT+RXRESULT",      1,   do_get_rx_result,
            "AT+RXRESULT        - get rx test result"
        },
        {
            "AT+RXRESTART",     1,   do_restart_rx_test,
            "AT+RXRESTART       - restart rx test"
        },
        {
            "AT+WTPOFS_DEF",    2,   do_write_tx_power_offset_flash,
            "AT+WTPOFS_DEF      - write tx power offset of g/n mode to flash"
        },
        {
            "AT+RTPOFS_DEF",    1,   do_read_tx_power_offset_flash,
            "AT+RTPOFS_DEF      - read tx power offset of g/n mode from flash"
        },
    };
    j = sizeof(cmd_list)/sizeof(cmd_tbl_t);
    for(i=0; i<j; i++){
        if(0 != console_cmd_add(&cmd_list[i])) {
            RDA_AT_PRINT("Add cmd failed\r\n");
        }
    }
}

void start_at(void)
{
    console_init();
    add_cmd();
}

