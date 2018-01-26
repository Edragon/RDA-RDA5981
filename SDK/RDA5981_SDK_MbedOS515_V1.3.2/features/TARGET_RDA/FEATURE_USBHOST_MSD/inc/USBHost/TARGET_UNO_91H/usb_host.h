#if defined(TARGET_UNO_91H)

#define BIT24                    (1ul << 24)
#define BIT25                    (1ul << 25)
#define BIT26                    (1ul << 26)
#define BIT27                    (1ul << 27)
#define BIT28                    (1ul << 28)
#define BIT29                    (1ul << 29)
#define BIT30                    (1ul << 30)
#define BIT31                    (1ul << 31)

#define RF_SPI_BUSY_BIT          BIT31
#define RF_SPI_CLK_RATE_OFST     (28)
#define RF_SPI_SEL_BIT           BIT27
#define RF_SPI_CLK_ENV_BIT       BIT26
#define RF_SPI_START_BIT         BIT25
#define RF_SPI_RW_BIT            BIT24
#define RF_SPI_ADDR_OFST         (16)
#define RF_SPI_DATA_OFST         (0)

#define RF_SPI_CLK_DATA_WIDTH    (3)
#define RF_SPI_ADDR_WIDTH        (8)
#define RF_SPI_DATA_WIDTH        (16)

#define RDA_AHB1_EXIF_SPI2_CTRL_DATA_REG (RDA_SPI0_BASE + 0x001C)
#define RDA_READ_REG32(REG) (*(volatile uint32_t *)(REG))
#define RDA_WRITE_REG32(REG,VAL) ((*(volatile uint32_t *)(REG)) = (uint32_t)(VAL)

#define ADDR2REG(addr)          (*((volatile unsigned int *)(addr)))

#define RF_SPI_REG              ADDR2REG(0x4001301CUL)

#endif
