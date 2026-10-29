#ifndef APP_TEMP_LED_H
#define APP_TEMP_LED_H


#include <stdint.h>

#define TEMP_RED_MIN_X10 300
#define TEMP_GREEN_MIN_X10 260
#define TEMP_HYST_X10 3

typedef enum {
	TEMP_ZONE_NONE,
	TEMP_ZONE_BLUE,
	TEMP_ZONE_GREEN,
	TEMP_ZONE_RED,
}app_temp_zone_t;

app_temp_zone_t app_temp_led_next_zone(app_temp_zone_t current, int32_t temp_x10);

void app_temp_led_show(app_temp_zone_t zone);

const char *app_temp_led_zone_to_string(app_temp_zone_t zone);

#endif
