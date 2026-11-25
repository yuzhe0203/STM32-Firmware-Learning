#include <stdint.h>
#include "temp_zone.h"

/* Public Part */
temp_zone_t temp_zone_next(temp_zone_t current, int32_t temp_x10) {
	switch(current) {

	case TEMP_ZONE_NONE:

	case TEMP_ZONE_BLUE:
		if (temp_x10 >= TEMP_RED_MIN_X10) {
			return TEMP_ZONE_RED;
		}
		else if (temp_x10 >= TEMP_GREEN_MIN_X10) {
			return TEMP_ZONE_GREEN;
		}
		else {
			return TEMP_ZONE_BLUE;
		}

	case TEMP_ZONE_GREEN:
		if (temp_x10 >= TEMP_RED_MIN_X10) {
			return TEMP_ZONE_RED;
		}
		else if (temp_x10 < (TEMP_GREEN_MIN_X10 - TEMP_HYST_X10)) {
			return TEMP_ZONE_BLUE;
		}
		else {
			return TEMP_ZONE_GREEN;
		}

	case TEMP_ZONE_RED:
		if (temp_x10 < TEMP_GREEN_MIN_X10 - TEMP_HYST_X10) {
			return TEMP_ZONE_BLUE;
		}
		else if (temp_x10 < (TEMP_RED_MIN_X10 - TEMP_HYST_X10)) {
			return TEMP_ZONE_GREEN;
		}
		else {
			return TEMP_ZONE_RED;
		}

	default:
		return TEMP_ZONE_NONE;

	}
}

const char *temp_zone_to_string(temp_zone_t zone) {
	switch(zone) {
	case TEMP_ZONE_NONE:
		return "None";
		break;

	case TEMP_ZONE_BLUE:
		return "Blue";
		break;

	case TEMP_ZONE_GREEN:
		return "Green";
		break;

	case TEMP_ZONE_RED:
		return "Red";
		break;

	default:
		return "Unknown";
		break;

	}
}
