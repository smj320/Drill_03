#ifndef _BM1422_H_
#define _BM1422_H_

#include "main.h"

#define BM1422_ADR                  0x0E

uint8_t BM1422_Init(void);

void BM1422_read_mag(int16_t mag[]);
void BM1422_dump();

#endif // _BM1422_H_
