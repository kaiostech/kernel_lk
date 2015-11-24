

typedef struct {
        uint32_t    CR1;    //Control Register 1 - 0x00
        uint32_t    CR2;    //Control Register 2 - 0x04
        uint32_t    SR;     //Status Register - 0x08
        uint32_t    DR;     //Data Register - 0x0C
        uint32_t    CRCPR;  //CRC Polynomial Register 0x10
        uint32_t    RXCRCR; //RX CRC Register 0x14
        uint32_t    TXCRCR; //TX CRC Register 0x18
        uint32_t    I2SCFGR;    //I2S Configuration Register 0x1C
        uint32_t    I2SPR;      //I2S Prescaler Register 0x20
} SPI_Type;

#define SPI1_BASE 0x40013000;
#define I2S1_BASE 0x40013000;
#define SPI2_BASE 0x40003800;
#define I2S2_BASE 0x40003800;
#define SPI3_BASE 0x40003C00;
#define I2S3_BASE 0x40003C00;
#define SPI4_BASE 0x40014000;
#define SPI5_BASE 0x40015000;
#define SPI6_BASE 0x40015400;

#define SPI1        ((SPI_Type *) SPI1_BASE)
#define I2S1        ((SPI_Type *) I2S1_BASE)
#define SPI2        ((SPI_Type *) SPI2_BASE)
#define I2S2        ((SPI_Type *) I2S2_BASE)
#define SPI3        ((SPI_Type *) SPI3_BASE)
#define I2S3        ((SPI_Type *) I2S3_BASE)
#define SPI4        ((SPI_Type *) SPI4_BASE)
#define SPI5        ((SPI_Type *) SPI5_BASE)
#define SPI6        ((SPI_Type *) SPI6_BASE)


/**
 *  CR1 bitfields
 */

#define     CR1_BIDIMODE    15
#define     CR1_BIDIOE      14
#define     CR1_CRCEN       13
#define     CR1_CRCNEXT     12
#define     CR1_CRCL        11
#define     CR1_RXONLY      10
#define     CR1_SSM         9
#define     CR1_SSI         8
#define     CR1_LSBFIRST    7
#define     CR1_SPE         6
#define     CR1_BR          3
#define     CR1_MSTR        2
#define     CR1_CPOL        1
#define     CR1_CPHA        0

/**
 *  CR2 bitfields
 */

#define     CR2_LDMA_TX     14
#define     CR2_LDMA_RX     13
#define     CR2_FRXTH       12
#define     CR2_DS          8
#define     CR2_TXEIE       7
#define     CR2_RXNEIE      6
#define     CR2_ERRIE       5
#define     CR2_FRF         4
#define     CR2_NSSP        3
#define     CR2_SSOE        2
#define     CR2_TXDMAEN     1
#define     CR2_RXDMAEN     0

/**
 *  SR bitfields
 */

#define     SR_FTLVL        11
#define     SR_FRLVL        9
#define     SR_FRE          8
#define     SR_BSY          7
#define     SR_OVR          6
#define     SR_MODF         5
#define     SR_CRCERR       4
#define     SR_UDR          3
#define     SR_CHSIDE       2
#define     SR_TXE          1
#define     SR_RXNE         0

/**
 *  I2SCFGR bitfields
 */

#define     I2SCFGR_ASTRTEN 12
#define     I2SCFGR_I2SMOD  11
#define     I2SCFGR_I2SE    10
#define     I2SCFGR_I2SCFG  8
#define     I2SCFGR_PCMSYNC 7
#define     I2SCFGR_I2SSTD  4
#define     I2SCFGR_CKPOL   3
#define     I2SCFGR_DATLEN  1
#define     I2SCFGR_CHLEN   0

/**
 *  I2SPR
 */

#define     I2SPR_MCKOE     9
#define     I2SPR_ODD       8
#define     I2SPR_I2SDIV    0


