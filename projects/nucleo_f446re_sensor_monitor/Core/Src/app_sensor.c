#include <main.h>
#include <stdbool.h>
#include "cmsis_os.h"
#include "app_sensor.h"

#define ADC_BUFFER_LEN 1u
#define SENSOR_FLAG_NEW_SAMPLE 0x01u
#define TEMP_AVG_LEN 8u
#define SENSOR_TIMEOUT_MS (SAMPLE_MAX_PERIOD_MS * 2u)
#define TS_CAL1_ADDRESS 0x1FFF7A2Cu
#define TS_CAL2_ADDRESS 0x1FFF7A2Eu
#define TS_CAL1_TEMP_C 30u
#define TS_CAL2_TEMP_C 110u
#define TIM_TICKS_PER_MS 10u // Assumes PSC = 8399 and 84 MHz timer clock (10 kHz tick)

static ADC_HandleTypeDef *adc = NULL;
static TIM_HandleTypeDef *tim = NULL;
static osMutexId_t mutex = NULL;
static volatile osThreadId_t task_id = NULL;
static volatile uint16_t adc_buffer[ADC_BUFFER_LEN] = {0};
static volatile uint32_t isr_count = 0u;
static uint32_t period_ms = SAMPLE_DEFAULT_PERIOD_MS;
static app_sensor_snapshot_t shared = {0};

static int32_t raw_to_temp_x10(uint16_t raw);
static void apply_period(uint32_t ms);
static void publish(const app_sensor_snapshot_t *s);

/* Public Part */
void app_sensor_init(ADC_HandleTypeDef *hadc, TIM_HandleTypeDef *htim) {
	adc = hadc;
	tim = htim;
	mutex = osMutexNew(NULL);

	if (mutex == NULL) {
		Error_Handler();
	}

}

bool app_sensor_set_period(uint32_t ms) {
	if ((ms > SAMPLE_MAX_PERIOD_MS) || (ms < SAMPLE_MIN_PERIOD_MS)) {
		return false;
	}
	else {
		period_ms = ms;
		apply_period(period_ms);
		return true;
	}
}

uint32_t app_sensor_get_period(void) {
	return period_ms;
}

void app_sensor_read(app_sensor_snapshot_t *out) {
	osMutexAcquire(mutex, osWaitForever);
	*out = shared;
	osMutexRelease(mutex);
}

void app_sensor_task() {
	task_id = osThreadGetId();

	if (HAL_ADC_Start_DMA(adc, (uint32_t *)adc_buffer, ADC_BUFFER_LEN) != HAL_OK) {
		Error_Handler();
	}

	apply_period(period_ms);

	if (HAL_TIM_Base_Start(tim) != HAL_OK) {
		Error_Handler();
	}

	int32_t history[TEMP_AVG_LEN] = {0};
	uint32_t index = 0u;
	uint32_t filled = 0u;
	int32_t sum = 0;
	uint32_t flags;
	app_sensor_snapshot_t local = {0};

	for (;;) {
		flags = osThreadFlagsWait(SENSOR_FLAG_NEW_SAMPLE, osFlagsWaitAny, SENSOR_TIMEOUT_MS);

		if ((flags & osFlagsError) != 0u) {
			local.timeouts++;
		}
		else {
			uint16_t raw = adc_buffer[0];
			int32_t temp = raw_to_temp_x10(raw);

			sum = (sum - history[index]) + temp;
			history[index] = temp;
			index = (index + 1u) & (TEMP_AVG_LEN - 1u);

			if (filled < TEMP_AVG_LEN) {
				filled++;
			}

			local.raw = raw;
			local.temp_x10 = temp;
			local.avg_x10 = sum / (int32_t)filled;
			local.processed++;
			local.samples = isr_count;

			publish(&local);
		}
	}
}

/* Private Part */

static int32_t raw_to_temp_x10(uint16_t raw) {
	int32_t cal1 = *(const uint16_t *)TS_CAL1_ADDRESS;
	int32_t cal2 = *(const uint16_t *)TS_CAL2_ADDRESS;
	int32_t raw_s32 = (int32_t)raw;

	if (cal1 == cal2) {
		return 0;
	}

	return TS_CAL1_TEMP_C * 10 + ((TS_CAL2_TEMP_C - TS_CAL1_TEMP_C) * 10) * (raw_s32 - cal1) / (cal2 - cal1);

}

static void apply_period(uint32_t ms) {
	uint32_t arr = (ms * TIM_TICKS_PER_MS) - 1;

	__HAL_TIM_SET_AUTORELOAD(tim, arr);

	__HAL_TIM_SET_COUNTER(tim, 0u);
}

static void publish(const app_sensor_snapshot_t *s) {
	osMutexAcquire(mutex, osWaitForever);
	shared = *s;
	osMutexRelease(mutex);
}

/* System Configuration */

void HAL_ADC_ConvCpltCallback(ADC_HandleTypeDef *hadc) {
	if (hadc != adc) {
		return;
	}

	isr_count++;

	if (task_id != NULL) {
		osThreadFlagsSet(task_id, SENSOR_FLAG_NEW_SAMPLE);
	}
}
