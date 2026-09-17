# Bit Operations Practice

A small C utility for manipulating individual bits in a `uint32_t` value.

## Features

- Set a bit
- Clear a bit
- Check whether a bit is set
- Toggle a bit
- Validate `NULL` pointers and bit indexes from `0` to `31`

## Build and Run

```bash
gcc -std=c11 -Wall -Wextra -Werror -g \
  bit_ops.c test_bit_ops.c \
  -o test_bit_ops.exe

./test_bit_ops.exe
```

## Firmware Relevance

Microcontroller peripherals are controlled through registers made of bits.
These operations are the basis for setting configuration flags, checking status flags,
and toggling GPIO output states.
