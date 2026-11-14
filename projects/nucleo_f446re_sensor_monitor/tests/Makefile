CC = gcc
CFLAGS = -std=c11 -Wall -Wextra -Werror -I../Core/Inc -Iunity

SRCS = unity/unity.c test_temp_zone.c ../Core/Src/temp_zone.c
HDRS = ../Core/Inc/temp_zone.h
TARGET = build/test_temp_zone

.PHONY: all test clean

all: $(TARGET)

$(TARGET): $(SRCS) $(HDRS)
	mkdir -p build
	$(CC) $(CFLAGS) $(SRCS) -o $(TARGET)
test: all
	./$(TARGET)
clean:
	rm -rf build