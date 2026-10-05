# STM32 Firmware Learning

A hands-on learning repository for STM32 firmware development.

## Goal

Build STM32 sensor monitor using:

- C
- STM32CubeIDE
- GPIO, UART, Timer, ADC, and I2C
- FreeRTOS tasks and queues
- Error handling and watchdog recovery

## Hardware and Tools

- Board: NUCLEO-F446RE (MCU: STM32F446RE)
- STM32CubeMX: peripheral configuration and code generation
- STM32CubeIDE: build, flash, and debug
- Debug/programmer: onboard ST-LINK (`CN1` Mini-USB)
- Serial terminal: PowerShell script using `System.IO.Ports.SerialPort`
  (ST-LINK Virtual COM Port, USART2, `115200`, 8-N-1, no flow control)

## Progress

- [ ] C bit operations
- [x] Circular buffer (RX and TX ring buffers in the UART DMA project)
- [x] UART logging (interrupt-driven and DMA-based command interface)
- [x] Timer-triggered ADC sampling with DMA (internal temperature sensor)
- [ ] I2C sensor
- [ ] FreeRTOS
- [ ] Final sensor-monitor project

Also done: persisting configuration to internal Flash
(`CONFIG SHOW`, `CONFIG SAVE`, and `CONFIG RESET`).

## Projects

All projects are under `projects/`.

| Project                           | Description                                                                                                                                                      |
| --------------------------------- | ---------------------------------------------------------------------------------------------------------------------------------------------------------------- |
| `nucleo_f446re_blink`             | Blink the user LED (`LD2`, `PA5`).                                                                                                                               |
| `nucleo_f446re_button_blink`      | Press the user button (`B1`, `PC13`, EXTI) to toggle between slow and fast blinking.                                                                             |
| `nucleo_f446re_uart_commands`     | Text command controller over the Virtual COM Port using UART receive interrupt (one byte at a time).                                                             |
| `nucleo_f446re_uart_dma_commands` | UART command controller built on circular RX DMA and a queued TX DMA, with a non-blocking blink scheduler, diagnostics, and Flash-backed configuration.          |
| `nucleo_f446re_adc_sampling`      | Current main project. Copy of the UART DMA project plus periodic ADC sampling of the internal temperature sensor (TIM2 trigger, ADC1, DMA) and an `ADC` command. |

### `nucleo_f446re_uart_dma_commands`

Data path:

```text
USART2 RX DMA (circular)
-> DMA RX buffer
-> software RX ring buffer
-> main-loop command parser
-> TX software ring buffer
-> USART2 TX DMA
-> TX-complete callback starts the next queued block
```

Commands (one per line, LF or CRLF, case-sensitive):

| Command              | Description                                                                           |
| -------------------- | ------------------------------------------------------------------------------------- |
| `HELP`               | List available commands.                                                              |
| `LED ON` / `LED OFF` | Set the LED and disable blink mode.                                                   |
| `STATUS`             | Show LED state and blink state.                                                       |
| `BLINK <100-2000>`   | Blink the LED with the given period in ms (non-blocking).                             |
| `BLINK OFF`          | Stop blinking and keep the current LED state.                                         |
| `STATS`              | Show uptime, RX events, RX dropped bytes, overlong commands, and TX dropped messages. |
| `RESET STATS`        | Reset the diagnostic counters (uptime is not reset).                                  |
| `CONFIG SHOW`        | Show the current blink period and the one stored in Flash.                            |
| `CONFIG SAVE`        | Save the current blink period to Flash (erase, write, read back, verify).             |
| `CONFIG RESET`       | Erase the stored configuration and return to the default period (500 ms).             |

Design notes:

- Callbacks only move bytes and update counters. Command parsing and other
  longer work happen in the main loop.
- A full reply is either queued completely or dropped completely when the TX
  ring buffer is full, and each drop is counted.
- Configuration is stored in Flash Sector 7 (`0x08060000`) with a magic number,
  version, and checksum. Invalid or blank data falls back to the default period
  (500 ms). The linker script `FLASH` length is limited to `384K` so program code
  can never overlap Sector 7.
- Saving and resetting both erase a whole 128 KB sector, so the CPU is paused
  for about a second or more while `CONFIG SAVE` or `CONFIG RESET` runs.
  `CONFIG RESET` only erases the sector, since a blank sector already means
  "no saved configuration".

### `nucleo_f446re_adc_sampling`

Starts from the UART DMA project, so all of its commands and the Flash-backed
configuration are still available. It adds periodic sampling of the internal
temperature sensor.

Sampling path:

```text
TIM2 update event (10 Hz)
-> TRGO triggers one ADC1 conversion (temperature sensor channel)
-> ADC1 DMA (circular, half-word) writes the result to RAM
-> conversion-complete callback increments a sample counter
-> main loop reads the latest value when asked
```

Additional command:

| Command | Description                                                                                              |
| ------- | -------------------------------------------------------------------------------------------------------- |
| `ADC`   | Show the latest raw ADC value, the temperature in degrees C, and the number of samples taken since boot. |

Design notes:

- The sampling time is set by a hardware timer, so it does not depend on how
  busy the main loop is. TIM2 runs from an 84 MHz timer clock with
  prescaler 8399 and period 999, which gives 10 Hz.
- The ADC sampling time is 480 cycles at a 21 MHz ADC clock (about 23 us),
  longer than the minimum the datasheet requires for the temperature sensor.
- Temperature is computed with integer math in units of 0.1 degree C, using the
  factory calibration values stored in the MCU (two-point linear interpolation).
  Floating point is avoided because `printf` float support is off by default in
  newlib-nano.
- Output text is plain ASCII. A non-ASCII degree sign showed up as `??` in the
  serial terminal.
- The internal sensor measures the chip temperature, which is usually a few
  degrees above room temperature. It is used to learn the sampling pipeline,
  not as the final sensor.
- The sample counter can fall slightly behind during `CONFIG SAVE` and
  `CONFIG RESET`, because the CPU is paused during Flash erase and the
  conversion-complete callback runs late. The DMA itself keeps running.

## Repository Notes

Committed: `.ioc`, `Core/`, `Drivers/`, `*.ld`, `.project`, `.cproject`, `.settings/`.

Ignored build and workspace output: `**/Debug/`, `**/Release/`, `.metadata/`,
`RemoteSystemsTempFiles/`, `*.exe`.

After regenerating code with CubeMX, re-check the IRQ and DMA handlers and the
`FLASH` length in the linker script.
