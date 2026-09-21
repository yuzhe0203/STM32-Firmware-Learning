# UART LED Controller Practice

A modular C practice project that simulates a UART-controlled LED.

## Architecture

```text
UART bytes
    ↓
Circular buffer
    ↓
Command-line assembly
    ↓
UART command parser
    ↓
LED state update
```

## Supported Commands

```text
LED ON
LED OFF
LED TOGGLE
STATUS
```

Commands are processed only after a newline (`\n`) is received.
CRLF line endings (`\r\n`) are also supported.

## Build and Run

```bash
gcc -std=c11 -Wall -Wextra -Werror -g \
  uart_led_controller.c \
  ../circular_buffer/circular_buffer.c \
  ../uart_command_parser/uart_command_parser.c \
  test_uart_led_controller.c \
  -o test_uart_led_controller.exe

./test_uart_led_controller.exe
```

## Firmware Relevance

This models a common STM32 firmware design:

- A UART interrupt or DMA callback receives bytes.
- Received bytes are buffered safely.
- The main loop processes complete commands.
- Parsed commands update application state.
- Hardware code later maps the LED state to a GPIO output.
