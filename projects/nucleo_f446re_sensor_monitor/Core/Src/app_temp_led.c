#include <main.h>
#include "app_temp_led.h"

/* Public Part */
app_temp_zone_t app_temp_led_next_zone(app_temp_zone_t current, int32_t temp_x10) {
	switch(current) {

	case TEMP_ZONE_NONE:
		if (temp_x10 >= 300) {
			return TEMP_ZONE_RED;
		}
		else if (temp_x10 >= 260) {
			return TEMP_ZONE_GREEN;
		}
		else {
			return TEMP_ZONE_BLUE;
		}
		break;

	case TEMP_ZONE_BLUE:
		if (temp_x10 >= 300) {
			return TEMP_ZONE_RED;
		}
		else if (temp_x10 >= 260) {
			return TEMP_ZONE_GREEN;
		}
		else {
			return TEMP_ZONE_BLUE;
		}
		break;

	case TEMP_ZONE_GREEN:
		if (temp_x10 >= 300) {
			return TEMP_ZONE_RED;
		}
		else if (temp_x10 < (TEMP_GREEN_MIN_X10 - TEMP_HYST_X10)) {
			return TEMP_ZONE_BLUE;
		}
		else {
			return TEMP_ZONE_GREEN;
		}
		break;

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
		break;

	default:
		return TEMP_ZONE_NONE;
		break;

	}
}

void app_temp_led_show(app_temp_zone_t zone) {
	uint32_t all_mask = (uint32_t)(LED_TEMP_RED_Pin | LED_TEMP_GREEN_Pin | LED_TEMP_BLUE_Pin);
	uint32_t on_mask = 0u;
	uint32_t off_mask = 0u;

	switch(zone) {

	case TEMP_ZONE_RED:
		on_mask = (uint32_t)LED_TEMP_RED_Pin;
		break;

	case TEMP_ZONE_GREEN:
		on_mask = (uint32_t)LED_TEMP_GREEN_Pin;
		break;

	case TEMP_ZONE_BLUE:
		on_mask = (uint32_t)LED_TEMP_BLUE_Pin;
		break;

	default:
		on_mask = 0u;
		break;

	}

	off_mask = all_mask & ~on_mask;

	LED_TEMP_RED_GPIO_Port->BSRR = on_mask | (off_mask << 16u);
}

const char *app_temp_led_zone_to_string(app_temp_zone_t zone) {
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

/* Private Part */

/* System Configuration */
