#include <stdbool.h>
#include "cmsis_os.h"
#include "app_mcp9808.h"

#define MCP9808_I2C_ADDRESS (0x18u << 1)
#define MCP9808_REG_AMBIENT 0x05u
#define MCP9808_I2C_TIMEOUT_MS 100u
#define MCP9808_READ_PERIOD_MS 1000u

static I2C_HandleTypeDef *i2c = NULL;
static osMutexId_t mutex = NULL;
static app_mcp9808_snapshot_t shared = {0};

static bool read_register(uint8_t reg, uint16_t *value);
static bool read_temp_x10(int32_t *out);
static void publish(app_mcp9808_snapshot_t *s);

/* Public Part */
void app_mcp9808_init(I2C_HandleTypeDef *hi2c) {
	i2c = hi2c;
	mutex = osMutexNew(NULL);
	shared.zone = TEMP_ZONE_NONE;

	if (mutex == NULL) {
		Error_Handler();
	}
}

void app_mcp9808_task(void) {
	app_mcp9808_snapshot_t local = {0};
	int32_t temp;
	uint32_t next_wake = osKernelGetTickCount();
	app_temp_zone_t zone = TEMP_ZONE_NONE;
	app_temp_led_show(zone);

	for (;;) {

		if (read_temp_x10(&temp)) {
			local.temp_x10 = temp;
			local.valid = true;
			local.ok_count++;
			local.zone = app_temp_led_next_zone(zone, temp);
		}
		else {
			local.valid = false;
			local.error_count++;
			local.zone = TEMP_ZONE_NONE;
		}

		publish(&local);
		app_temp_led_show(local.zone);
		next_wake += MCP9808_READ_PERIOD_MS;
		osDelayUntil(next_wake);

	}
}

void app_mcp9808_read(app_mcp9808_snapshot_t *out) {
	osMutexAcquire(mutex, osWaitForever);
	*out = shared;
	osMutexRelease(mutex);
}

/* Private Part */
static bool read_register(uint8_t reg, uint16_t *value) {
	uint8_t buf[2];

	if (HAL_I2C_Mem_Read(i2c, MCP9808_I2C_ADDRESS, reg, I2C_MEMADD_SIZE_8BIT, buf, 2, MCP9808_I2C_TIMEOUT_MS) != HAL_OK) {
		return false;
	}

	*value = ((uint16_t)buf[0] << 8) | buf[1];

	return true;
}

static bool read_temp_x10(int32_t *out) {
	uint16_t value;
	int32_t temp;

	if (!read_register(MCP9808_REG_AMBIENT, &value)) {
		return false;
	}

	value &= 0x1FFFu;
	temp = (int32_t)value;

	if (temp & 0x1000u) {
		temp -= 0x2000u;
	}

	temp = (temp * 10) /16;
	*out = temp;

	return true;
}

static void publish(app_mcp9808_snapshot_t *s) {
	osMutexAcquire(mutex, osWaitForever);
	shared = *s;
	osMutexRelease(mutex);
}

/* System Configuration */
