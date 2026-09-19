# UART Command Parser Practice

A small C parser that converts complete UART command lines into command enum values.

## Supported Commands

```text
LED ON
LED OFF
LED TOGGLE
STATUS
```

The parser accepts a command with no line ending, an LF line ending (`\n`), or a CRLF line ending (`\r\n`).

## Build and Run

```bash
gcc -std=c11 -Wall -Wextra -Werror -g \
  uart_command_parser.c test_uart_command_parser.c \
  -o test_uart_command_parser.exe

./test_uart_command_parser.exe
```

## Firmware Relevance

UART is commonly used to communicate with microcontrollers through a serial terminal, another MCU, or a PC application.

A typical firmware flow is:

```text
UART receives bytes
    ↓
Bytes are stored in a circular buffer
    ↓
A complete command line is extracted
    ↓
The command parser converts text to an enum
    ↓
Firmware performs an action, such as controlling an LED
```

This parser is designed to work later with the circular buffer and LED state machine exercises.
