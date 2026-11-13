#ifndef TEMP_ZONE_H
#define TEMP_ZONE_H

#define TEMP_RED_MIN_X10 300
#define TEMP_GREEN_MIN_X10 260
#define TEMP_HYST_X10 3

typedef enum {
	TEMP_ZONE_NONE,
	TEMP_ZONE_BLUE,
	TEMP_ZONE_GREEN,
	TEMP_ZONE_RED,
} temp_zone_t;

temp_zone_t temp_next_zone(temp_zone_t current, int32_t temp_x10);

const char *temp_zone_to_string(temp_zone_t zone);

#endif
