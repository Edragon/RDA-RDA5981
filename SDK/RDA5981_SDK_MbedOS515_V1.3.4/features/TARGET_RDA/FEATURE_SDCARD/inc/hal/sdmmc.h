#ifndef _SDMMC_H__
#define _SDMMC_H__

#include "global_macros.h"
#include "PinNames.h"

#define SDMMC_TEST_ENABLE 1

// =============================================================================
// TYPES
// =============================================================================

typedef struct
{
    UINT8   mid;
    UINT32  oid;
    UINT32  pnm1;
    UINT8   pnm2;
    UINT8   prv;
    UINT32  psn;
    UINT32  mdt;
    UINT8   crc;
} MCD_CID_FORMAT_T;


// =============================================================================
// MCD_CARD_STATE_T
// -----------------------------------------------------------------------------
/// The state of the card when receiving the command. If the command execution
/// causes a state change, it will be visible to the host in the response to
/// the next command. The four bits are interpreted as a binary coded number
/// between 0 and 15.
// =============================================================================
typedef enum
{
    MCD_CARD_STATE_IDLE    = 0,
    MCD_CARD_STATE_READY   = 1,
    MCD_CARD_STATE_IDENT   = 2,
    MCD_CARD_STATE_STBY    = 3,
    MCD_CARD_STATE_TRAN    = 4,
    MCD_CARD_STATE_DATA    = 5,
    MCD_CARD_STATE_RCV     = 6,
    MCD_CARD_STATE_PRG     = 7,
    MCD_CARD_STATE_DIS     = 8
} MCD_CARD_STATE_T;


// =============================================================================
// MCD_CARD_STATUS_T
// -----------------------------------------------------------------------------
/// Card status as returned by R1 reponses (spec V2 pdf p.)
// =============================================================================
typedef union
{
    UINT32 reg;
    struct
    {
        UINT32                          :3;
        UINT32 akeSeqError              :1;
        UINT32                          :1;
        UINT32 appCmd                   :1;
        UINT32                          :2;
        UINT32 readyForData             :1;
        MCD_CARD_STATE_T currentState   :4;
        UINT32 eraseReset               :1;
        UINT32 cardEccDisabled          :1;
        UINT32 wpEraseSkip              :1;
        UINT32 csdOverwrite             :1;
        UINT32                          :2;
        UINT32 error                    :1;
        UINT32 ccError                  :1;
        UINT32 cardEccFailed            :1;
        UINT32 illegalCommand           :1;
        UINT32 comCrcError              :1;
        UINT32 lockUnlockFail           :1;
        UINT32 cardIsLocked             :1;
        UINT32 wpViolation              :1;
        UINT32 eraseParam               :1;
        UINT32 eraseSeqError            :1;
        UINT32 blockLenError            :1;
        UINT32 addressError             :1;
        UINT32 outOfRange               :1;
    } fields;
} MCD_CARD_STATUS_T;

// =============================================================================
// MCD_CSD_T
// -----------------------------------------------------------------------------
/// This structure contains the fields of the MMC chip's register.
/// For more details, please refer to your MMC specification.
// =============================================================================
#define BOOL  BOOL_T
typedef struct
{                                   // Ver 2. // Ver 1.0 (if different)
    UINT8   csdStructure;           // 127:126
    UINT8   specVers;                   // 125:122
    UINT8   taac;                   // 119:112
    UINT8   nsac;                   // 111:104
    UINT8   tranSpeed;              // 103:96
    UINT16  ccc;                    //  95:84
    UINT8   readBlLen;              //  83:80
    BOOL    readBlPartial;          //  79:79
    BOOL    writeBlkMisalign;       //  78:78
    BOOL    readBlkMisalign;        //  77:77
    BOOL    dsrImp;                 //  76:76
    UINT32  cSize;                  //  69:48 // 73:62
    UINT8   vddRCurrMin;            //           61:59
    UINT8   vddRCurrMax;            //           58:56
    UINT8   vddWCurrMin;             //           55:53
    UINT8   vddWCurrMax;            //           52:50
    UINT8   cSizeMult;              //           49:47
    // FIXME
    UINT8   eraseBlkEnable;
    UINT8   eraseGrpSize;           //  ??? 46:42
    // FIXME
    UINT8   sectorSize;
    UINT8   eraseGrpMult;           //  ??? 41:37

    UINT8   wpGrpSize;              //  38:32
    BOOL    wpGrpEnable;            //  31:31
    UINT8   defaultEcc;             //  30:29
    UINT8   r2wFactor;              //  28:26
    UINT8   writeBlLen;             //  25:22
    BOOL    writeBlPartial;         //  21:21
    BOOL    contentProtApp;         //  16:16
    BOOL    fileFormatGrp;          //  15:15
    BOOL    copy;                   //  14:14
    BOOL    permWriteProtect;       //  13:13
    BOOL    tmpWriteProtect;        //  12:12
    UINT8   fileFormat;             //  11:10
    UINT8   ecc;                    //   9:8
    UINT8   crc;                    //   7:1
    /// This field is not from the CSD register.
    /// This is the actual block number.
    UINT32  blockNumber;
} MCD_CSD_T;
#undef BOOL

// =============================================================================
// MCD_ERR_T
// -----------------------------------------------------------------------------
/// Type used to describe the error status of the MMC driver.
// =============================================================================
typedef enum
{
    MCD_ERR_NO = 0,
    MCD_ERR_CARD_TIMEOUT = 1,
    MCD_ERR_DMA_BUSY = 3,
    MCD_ERR_CSD = 4,
    MCD_ERR_SPI_BUSY = 5,
    MCD_ERR_BLOCK_LEN = 6,
    MCD_ERR_CARD_NO_RESPONSE,
    MCD_ERR_CARD_RESPONSE_BAD_CRC,
    MCD_ERR_CMD,
    MCD_ERR_UNUSABLE_CARD,
    MCD_ERR_NO_CARD,
    MCD_ERR_NO_HOTPLUG,

    /// A general error value
    MCD_ERR,
} MCD_ERR_T;

// =============================================================================
// MCD_STATUS_T
// -----------------------------------------------------------------------------
/// Status of card
// =============================================================================
typedef enum
{
    // Card present and mcd is open
    MCD_STATUS_OPEN,
    // Card present and mcd is not open
    MCD_STATUS_NOTOPEN_PRESENT,
    // Card not present
    MCD_STATUS_NOTPRESENT,
    // Card removed, still open (please close !)
    MCD_STATUS_OPEN_NOTPRESENT
} MCD_STATUS_T ;

// =============================================================================
// MCD_CARD_SIZE_T
// -----------------------------------------------------------------------------
/// Card size
// =============================================================================
typedef struct
{
    UINT32 nbBlock;
    UINT32 blockLen;
} MCD_CARD_SIZE_T ;


// =============================================================================
// MCD_CARD_VER
// -----------------------------------------------------------------------------
/// Card version
// =============================================================================

typedef enum
{
    MCD_CARD_V1,
    MCD_CARD_V2
}MCD_CARD_VER;


// =============================================================================
// MCD_CARD_ID
// -----------------------------------------------------------------------------
/// Card version
// =============================================================================

typedef enum
{
    MCD_CARD_ID_0,
    MCD_CARD_ID_1,
    MCD_CARD_ID_NO,
}MCD_CARD_ID;

#ifdef __cplusplus
extern "C" {
#endif

void rda_sdmmc_init(PinName clk, PinName cmd, PinName d0, PinName d1, PinName d2, PinName d3);
int rda_sdmmc_open(void);
int rda_sdmmc_read_blocks(UINT8 *buffer, UINT32 block_start, UINT32 block_num);
int rda_sdmmc_write_blocks(CONST UINT8 *buffer, UINT32 block_start, UINT32 block_num);
int rda_sdmmc_get_csdinfo(UINT32 *block_size);
#if SDMMC_TEST_ENABLE
void rda_sdmmc_test(void);
#endif

#ifdef __cplusplus
}
#endif

#endif /* _SDMMC_H_ */

