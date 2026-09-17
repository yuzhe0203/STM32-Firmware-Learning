# Circular Buffer Practice

A fixed-capacity FIFO circular buffer for `uint8_t` data.

## Features

- Initialize a buffer
- Push bytes into the buffer
- Pop the oldest byte from the buffer
- Detect empty and full states
- Track the number of stored bytes
- Wrap indexes back to the start of the array
- Validate invalid pointers

## Build and Run

```bash
gcc -std=c11 -Wall -Wextra -Werror -g \
  circular_buffer.c test_circular_buffer.c \
  -o test_circular_buffer.exe

./test_circular_buffer.exe
```

## Firmware Relevance

Circular buffers are commonly used for UART receive data, DMA streams,
sensor samples, and communication between an interrupt handler and the main loop.
