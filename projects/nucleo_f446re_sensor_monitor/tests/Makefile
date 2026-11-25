CC     = gcc
CFLAGS = -std=c11 -Wall -Wextra -Werror -I../Core/Inc -Iunity

ZONE_SRCS = unity/unity.c test_temp_zone.c ../Core/Src/temp_zone.c
ZONE_HDRS = ../Core/Inc/temp_zone.h
ZONE_BIN  = build/test_temp_zone

CHIP_SRCS = unity/unity.c test_chip_temp.c ../Core/Src/chip_temp.c
CHIP_HDRS = ../Core/Inc/chip_temp.h
CHIP_BIN  = build/test_chip_temp

MCP_SRCS  = unity/unity.c test_mcp9808_conv.c ../Core/Src/mcp9808_conv.c
MCP_HDRS  = ../Core/Inc/mcp9808_conv.h
MCP_BIN   = build/test_mcp9808_conv

.PHONY: all test clean

all: $(ZONE_BIN) $(CHIP_BIN) $(MCP_BIN)

$(ZONE_BIN): $(ZONE_SRCS) $(ZONE_HDRS)
	mkdir -p build
	$(CC) $(CFLAGS) $(ZONE_SRCS) -o $(ZONE_BIN)

$(CHIP_BIN): $(CHIP_SRCS) $(CHIP_HDRS)
	mkdir -p build
	$(CC) $(CFLAGS) $(CHIP_SRCS) -o $(CHIP_BIN)

$(MCP_BIN): $(MCP_SRCS) $(MCP_HDRS)
	mkdir -p build
	$(CC) $(CFLAGS) $(MCP_SRCS) -o $(MCP_BIN)

test: all
	./$(ZONE_BIN)
	./$(CHIP_BIN)
	./$(MCP_BIN)

clean:
	rm -rf build