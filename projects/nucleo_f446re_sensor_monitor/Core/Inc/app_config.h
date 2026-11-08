#ifndef APP_CONFIG_H
#define APP_CONFIG_H

#include <stdbool.h>
#include <stdint.h>
#include "app_led.h"

#define CONFIG_DEFAULT_LED_MODE APP_LED_OFF
#define CONFIG_DEFAULT_BLINK_MS 0u
#define CONFIG_DEFAULT_SAMPLE_MS 100u

typedef struct {
	uint32_t led_mode;
	uint32_t blink_ms;
	uint32_t sample_ms;
}app_config_values_t;

bool app_config_read(app_config_values_t *out);

bool app_config_save(const app_config_values_t *in);

bool app_config_erase(void);

void app_config_get_defaults(app_config_values_t *out);

#endif
