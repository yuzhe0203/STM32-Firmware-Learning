#ifdef APP_MCP9808_H
#define APP_MCP9808_H
#endif

#include <main.h>
#include <stdbool.h>

typedef struct {
	int32_t temp_x10;
	bool valid;
	uint32_t ok_count;
	uint32_t error_count;
}app_mcp9808_snapshot_t;

void app_mcp9808_init(I2C_HandleTypeDef *hi2c);

void app_mcp9808_task(void);

void app_mcp9808_read(app_mcp9808_snapshot_t *out);
