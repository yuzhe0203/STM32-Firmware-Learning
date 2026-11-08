#include <main.h>
#include <stdbool.h>
#include <string.h>
#include "app_config.h"

#define CONFIG_FLASH_ADDRESS 0x08060000u
#define CONFIG_FALSH_SECTOR FLASH_SECTOR_7
#define CONFIG_MAGIC 0x20261028u
#define CONFIG_VERSION 1u

typedef struct {
	uint32_t magic;
	uint32_t version;
	app_config_values_t values;
	uint32_t checksum;
}app_config_t;

_Static_assert(sizeof(app_config_t) == 24, "app_config_t size changed, check Flash layout and checksum range.\r\n");

static uint32_t checksum(const app_config_t *config);
static bool is_valid(const app_config_t *config);
static bool erase_sector(void);

/* Public Part */

bool app_config_read(app_config_values_t *out) {
	const app_config_t *config = (const app_config_t *)CONFIG_FLASH_ADDRESS;

	if (!is_valid(config)) {
		return false;
	}

	*out = config->values;

	return true;
}

bool app_config_save(const app_config_values_t *in) {
	app_config_t new_config;
	bool success = false;

	FLASH_EraseInitTypeDef erase = {0};
	uint32_t sector_error = 0u;
	HAL_StatusTypeDef status;

	uint32_t index;
	const uint32_t *words;

	new_config.magic = CONFIG_MAGIC;
	new_config.version = CONFIG_VERSION;
	new_config.values = *in;
	new_config.checksum = checksum(&new_config);

	words = (const uint32_t *)&new_config;

	status = HAL_FLASH_Unlock();

	if (status == HAL_OK) {
		erase.TypeErase = FLASH_TYPEERASE_SECTORS;
		erase.Sector = CONFIG_FALSH_SECTOR;
		erase.NbSectors = 1u;
		erase.VoltageRange = FLASH_VOLTAGE_RANGE_3;

		status = HAL_FLASHEx_Erase(&erase, &sector_error);

		if (status == HAL_OK) {
			for (index = 0u; index < (sizeof(app_config_t) / sizeof(uint32_t)); index++) {
				status = HAL_FLASH_Program(FLASH_TYPEPROGRAM_WORD, CONFIG_FLASH_ADDRESS + index*4u, words[index]);
				if (status != HAL_OK) {
					break;
				}
			}
		}
	}

	HAL_FLASH_Lock();

	if (status == HAL_OK) {
		const app_config_t *stored = (const app_config_t *)CONFIG_FLASH_ADDRESS;
		if (is_valid(stored) && memcmp(stored, &new_config, sizeof(app_config_t)) == 0) {
			success = true;
		}
	}

	return success;
}

bool app_config_erase(void) {
	const app_config_t *config = (const app_config_t *)CONFIG_FLASH_ADDRESS;

	if (!erase_sector()) {
		return false;
	}

	if (is_valid(config)) {
		return false;
	}
	else {
		return true;
	}

}

void app_config_get_defaults(app_config_values_t *out) {
	if (out == NULL) {
		Error_Handler();
	}

	out->blink_ms = CONFIG_DEFAULT_BLINK_MS;
	out->led_mode = (uint32_t)CONFIG_DEFAULT_LED_MODE;
	out->sample_ms = CONFIG_DEFAULT_SAMPLE_MS;

}

/* Private Part */

static uint32_t checksum(const app_config_t *config) {
	return config->magic ^ config->values.blink_ms ^ config->values.led_mode ^ config->values.sample_ms ^ config->version ^ 1028u;
}

static bool is_valid(const app_config_t *config) {
	if (config->magic != CONFIG_MAGIC) {
		return false;
	}
	else if (config->checksum != checksum(config)) {
		return false;
	}
	else if (config->version != CONFIG_VERSION) {
		return false;
	}
	else {
		return true;
	}
}

static bool erase_sector(void) {
	bool success = false;

	FLASH_EraseInitTypeDef erase = {0};
	uint32_t sector_error = 0u;
	HAL_StatusTypeDef status;

	status = HAL_FLASH_Unlock();

	if (status == HAL_OK) {
		erase.TypeErase = FLASH_TYPEERASE_SECTORS;
		erase.Sector = CONFIG_FALSH_SECTOR;
		erase.NbSectors = 1u;
		erase.VoltageRange = FLASH_VOLTAGE_RANGE_3;

		status = HAL_FLASHEx_Erase(&erase, &sector_error);
	}

	HAL_FLASH_Lock();

	if (status == HAL_OK) {
		const app_config_t *stored = (const app_config_t *)CONFIG_FLASH_ADDRESS;

		if (!is_valid(stored)) {
			success = true;
		}
	}

	return success;
}

/* System Configuration */
