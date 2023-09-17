#ifndef _BM1422_H_
#define _BM1422_H_

#include "main.h"

#define BM1422_ADR                  0x0E

// Error Number
#define BM1422AGMV_OK                 (0)
#define BM1422AGMV_COMM_ERROR         (-1)
#define BM1422AGMV_WAI_ERROR          (-2)

// Slave Address
#define BM1422AGMV_SLAVE_ADDRESS_0E   (0x0E)
#define BM1422AGMV_SLAVE_ADDRESS_0F   (0x0F)
#define BM1422AGMV_ADDR_HIGH          (true)
#define BM1422AGMV_ADDR_LOW           (false)

// Register Address
#define BM1422AGMV_WIA                (0x0F)
#define BM1422AGMV_DATAX              (0x10)
#define BM1422AGMV_STA1               (0x18)
#define BM1422AGMV_CNTL1              (0x1B)
#define BM1422AGMV_CNTL2              (0x1C)
#define BM1422AGMV_CNTL3              (0x1D)
#define BM1422AGMV_AVE_A              (0x40)
#define BM1422AGMV_CNTL4              (0x5C)

// Register Parameter
#define BM1422AGMV_STA1_RD_DRDY_OK    (1 << 6)
#define BM1422AGMV_CNTL1_FS1_SINGLE   (1 << 1)
#define BM1422AGMV_CNTL1_RST_LV_LOW   (0 << 5)
#define BM1422AGMV_CNTL1_OUT_BIT      (1 << 6)
#define BM1422AGMV_CNTL1_PC1_ACTIVE   (1 << 7)
#define BM1422AGMV_CNTL2_DREN_DISABLE (0 << 3)
#define BM1422AGMV_CNTL3_FORCE        (1 << 6)
#define BM1422AGMV_AVE_A_4            (0 << 2)

// Register Value(write)
#define BM1422AGMV_CNTL1_VAL          (BM1422AGMV_CNTL1_PC1_ACTIVE | BM1422AGMV_CNTL1_OUT_BIT | BM1422AGMV_CNTL1_RST_LV_LOW | BM1422AGMV_CNTL1_FS1_SINGLE)
#define BM1422AGMV_CNTL2_VAL          (BM1422AGMV_CNTL2_DREN_DISABLE)
#define BM1422AGMV_CNTL3_VAL          (BM1422AGMV_CNTL3_FORCE)
#define BM1422AGMV_CNTL4_VAL          (0x0000)
#define BM1422AGMV_AVE_A_VAL          (BM1422AGMV_AVE_A_4)

// Register Value(read)
#define BM1422AGMV_WAI_VAL            (0x41)
// Meas Time
#define BM1422AGMV_MEAS_TIME          (3)
// Mask
#define BM1422AGMV_CNTL1_OUT_BIT_MASK (0x40)
// Version
#define BM1422AGMV_DRIVER_VERSION     (1.1F)
// Sensitivity
#define BM1422AGMV_14BIT_SENSITIVITY  (24)
#define BM1422AGMV_12BIT_SENSITIVITY  (6)
// Others
#define BM1422AGMV_RAW_DATA_SIZE      (6)
#define BM1422AGMV_DATA_SIZE          (3)

// Register Value(write)
#define BM1422AGMV_CNTL1_VAL (BM1422AGMV_CNTL1_PC1_ACTIVE | BM1422AGMV_CNTL1_OUT_BIT | BM1422AGMV_CNTL1_RST_LV_LOW | BM1422AGMV_CNTL1_FS1_SINGLE)
#define BM1422AGMV_CNTL2_VAL (BM1422AGMV_CNTL2_DREN_DISABLE)
#define BM1422AGMV_CNTL3_VAL (BM1422AGMV_CNTL3_FORCE)
#define BM1422AGMV_CNTL4_VAL (0x0000)
#define BM1422AGMV_AVE_A_VAL (BM1422AGMV_AVE_A_4)

int BM1422_Init();
int BM1422_getVal(int16_t xyz[]);
void BM1422_dump();

#endif // _BM1422_H_
