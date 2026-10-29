#ifndef APP_SENSOR_H
#define APP_SENSOR_H

#include <main.h>
#include <stdbool.h>

#define SAMPLE_MIN_PERIOD_MS 100u
#define SAMPLE_MAX_PERIOD_MS 10000u
#define SAMPLE_DEFAULT_PERIOD_MS 100u

typedef struct {
	uint16_t raw;
	int32_t temp_x10;
	int32_t avg_x10;
	uint32_t samples;
	uint32_t processed;
	uint32_t timeouts;
}app_sensor_snapshot_t;

void app_sensor_init(ADC_HandleTypeDef *hadc, TIM_HandleTypeDef *htim);

void app_sensor_task(void);

bool app_sensor_set_period(uint32_t ms);

uint32_t app_sensor_get_period(void);

void app_sensor_read(app_sensor_snapshot_t *out);

#endif
