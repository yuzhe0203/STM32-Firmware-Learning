#ifndef CHIP_TEMP_H
#define CHIP_TEMP_H

#include <stdint.h>

#define CHIP_TEMP_CAL1_C ((int32_t)30)
#define CHIP_TEMP_CAL2_C ((int32_t)110)

int32_t chip_temp_from_raw(uint16_t raw, uint16_t cal1, uint16_t cal2);

#endif
