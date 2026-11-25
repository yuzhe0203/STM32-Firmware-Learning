#include "chip_temp.h"

/* Public Part */
int32_t chip_temp_from_raw(uint16_t raw, uint16_t cal1, uint16_t cal2) {
	int32_t raw_s32 = (int32_t)raw;
	int32_t cal1_s32 = (int32_t)cal1;
	int32_t cal2_s32 = (int32_t)cal2;

	if (cal1_s32 == cal2_s32) {
		return 0;
	}

	return CHIP_TEMP_CAL1_C * 10 + ((CHIP_TEMP_CAL2_C - CHIP_TEMP_CAL1_C) * 10) * (raw_s32 - cal1_s32) / (cal2_s32 - cal1_s32);
}

/* Private Part */

/* System Configuration */
