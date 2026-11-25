#include "mcp9808_conv.h"

#define MCP9808_TEMP_MASK 0x1FFFu
#define MCP9808_SIGN_BIT 0x1000u
#define MCP9808_SIGN_OFFSET 0x2000

/* Public Part */
int32_t mcp9808_conv_to_x10(uint16_t reg_value) {
	uint32_t raw = (uint32_t)reg_value & MCP9808_TEMP_MASK;

	int32_t value = (int32_t)raw;

	if ((raw & MCP9808_SIGN_BIT) != 0u) {
		value -= MCP9808_SIGN_OFFSET;
	}

	return value * 10 / 16;
}

/* Private Part */

/* System Configuration */
