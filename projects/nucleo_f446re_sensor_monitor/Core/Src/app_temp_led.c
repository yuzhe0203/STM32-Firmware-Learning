#include <main.h>
#include "app_temp_led.h"

/* Public Part */
void app_temp_led_show(temp_zone_t zone) {
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

/* Private Part */

/* System Configuration */
