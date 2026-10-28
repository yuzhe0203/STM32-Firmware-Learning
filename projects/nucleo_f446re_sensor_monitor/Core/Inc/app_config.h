#ifdef APP_CONFIG_H
#define APP_CONFIG_H
#endif

#include <stdbool.h>
#include <stdint.h>

typedef struct {
	uint32_t led_mode;
	uint32_t blink_ms;
	uint32_t sample_ms;
}app_config_values_t;

bool app_config_read(app_config_values_t *out);

bool app_config_save(const app_config_values_t *in);

bool app_config_erase(void);
