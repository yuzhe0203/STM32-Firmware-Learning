# STM32 Firmware Learning

English | [繁體中文](README.zh-TW.md)

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
- External parts (later projects): breadboard LEDs x5, MCP9808 temperature
  sensor module

## Progress

- [x] C bit operations (register-level GPIO project)
- [x] Circular buffer (RX and TX ring buffers in the UART DMA project)
- [x] UART logging (interrupt-driven and DMA-based command interface)
- [x] Timer-triggered ADC sampling with DMA (internal temperature sensor)
- [x] I2C sensor (MCP9808)
- [x] FreeRTOS (tasks, queues, thread flags, mutexes)
- [x] Final sensor-monitor project

Also done: persisting configuration to internal Flash
(`CONFIG SHOW`, `CONFIG SAVE`, and `CONFIG RESET`), including the blink period
and the ADC sampling period, and an independent watchdog (IWDG) with per-task
supervision in the sensor-monitor project.

## Projects

All projects are under `projects/`.

| Project                           | Description                                                                                                                                                                              |
| --------------------------------- | ---------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------- |
| `nucleo_f446re_blink`             | Blink the user LED (`LD2`, `PA5`).                                                                                                                                                       |
| `nucleo_f446re_button_blink`      | Press the user button (`B1`, `PC13`, EXTI) to toggle between slow and fast blinking.                                                                                                     |
| `nucleo_f446re_uart_commands`     | Text command controller over the Virtual COM Port using UART receive interrupt (one byte at a time).                                                                                     |
| `nucleo_f446re_uart_dma_commands` | UART command controller built on circular RX DMA and a queued TX DMA, with a non-blocking blink scheduler, diagnostics, and Flash-backed configuration.                                  |
| `nucleo_f446re_adc_sampling`      | Copy of the UART DMA project plus periodic ADC sampling of the internal temperature sensor (TIM2 trigger, ADC1, DMA), an `ADC` command, and a sampling period saved in Flash.            |
| `nucleo_f446re_register_gpio`     | GPIO without HAL: configure and drive pins by writing `MODER`, `PUPDR`, `BSRR`, and reading `IDR` directly.                                                                              |
| `nucleo_f446re_freertos`          | FreeRTOS (CMSIS-RTOS v2) basics: tasks, message queues, thread flags, and mutexes.                                                                                                       |
| `nucleo_f446re_i2c_mcp9808`       | Read ambient temperature from an MCP9808 over I2C1 and convert the 13-bit two's complement value.                                                                                        |
| `nucleo_f446re_sensor_monitor`    | **Current main project.** Modular FreeRTOS rewrite combining UART commands, LEDs, ADC chip temperature, MCP9808 ambient temperature, temperature indicator LEDs, Flash config, and IWDG. |

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
TIM2 update event (default 10 Hz, set by SAMPLE)
-> TRGO triggers one ADC1 conversion (temperature sensor channel)
-> ADC1 DMA (circular, half-word) writes the result to RAM
-> conversion-complete callback increments a sample counter
-> main loop reads the latest value when asked
```

Additional and changed commands:

| Command              | Description                                                                                              |
| -------------------- | -------------------------------------------------------------------------------------------------------- |
| `ADC`                | Show the latest raw ADC value, the temperature in degrees C, and the number of samples taken since boot. |
| `SAMPLE <100-10000>` | Set the sampling period in ms. Takes effect immediately.                                                 |
| `CONFIG SHOW`        | Show the current and stored values of both the blink period and the sampling period.                     |
| `CONFIG SAVE`        | Save both periods to Flash.                                                                              |
| `CONFIG RESET`       | Erase the stored configuration and return to the defaults (blink 500 ms, sampling 100 ms).               |

Design notes:

- The sampling time is set by a hardware timer, so it does not depend on how
  busy the main loop is. TIM2 runs from an 84 MHz timer clock with
  prescaler 8399, so it counts at 10 kHz (10 ticks per ms). The default
  period 999 gives 10 Hz.
- `SAMPLE` writes the auto-reload register as `ms * 10 - 1` and then resets
  the counter to 0. Auto-reload preload is off, so without the reset the
  counter could already be past a new, smaller limit and would run until the
  32-bit counter wraps, which would stop sampling for days.
- The configuration is now version 2 and also stores the sampling period.
  Version 1 data in Flash fails validation and the defaults are used, which is
  the intended upgrade behavior. The stored period is applied to TIM2 at boot
  before the timer starts, and `CONFIG RESET` applies the default to TIM2
  right away.
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

### `nucleo_f446re_register_gpio`

Practice project for register-level GPIO access, without the HAL GPIO functions.

- `MODER` uses 2 bits per pin, so a pin's field is at bit `2 * pin`.
- `PUPDR` selects pull-up or pull-down for the input pin.
- `BSRR` sets a pin with the low 16 bits and resets it with the high 16 bits,
  in one atomic write (no read-modify-write).
- `IDR` is read to get the input level.

### `nucleo_f446re_freertos`

Practice project for FreeRTOS through the CMSIS-RTOS v2 API: creating tasks,
passing data with message queues, waking a task from an interrupt with thread
flags, and protecting shared data with a mutex.

### `nucleo_f446re_i2c_mcp9808`

Standalone I2C project that reads the MCP9808 ambient temperature register.

Wiring:

| MCP9808 | NUCLEO-F446RE    |
| ------- | ---------------- |
| VIN     | 3V3              |
| GND     | GND              |
| SCL     | `PB8` (`CN10-3`) |
| SDA     | `PB9` (`CN10-5`) |

- I2C1 at 100 kHz, 7-bit address `0x18`, register `0x05`, 2 bytes.
- The value is 13-bit two's complement in units of 1/16 degree C: mask with
  `0x1FFF`, subtract `0x2000` if bit 12 is set, then convert to units of
  0.1 degree C with `* 10 / 16`.

### `nucleo_f446re_sensor_monitor`

The final project. A modular rewrite on FreeRTOS that combines everything
above: UART commands over DMA, alternating blink LEDs, chip temperature from
the ADC, ambient temperature from the MCP9808, red/green/blue temperature
indicator LEDs, Flash-backed configuration, reset-cause reporting, and an
independent watchdog with per-task supervision.

Wiring (MCP9808 as above):

| Pin    | Label            | Use                               |
| ------ | ---------------- | --------------------------------- |
| `PB12` | `LED_BLINK_A`    | Blink LED A                       |
| `PB13` | `LED_BLINK_B`    | Blink LED B (opposite phase of A) |
| `PB14` | `LED_TEMP_RED`   | Ambient >= 30.0 C                 |
| `PB15` | `LED_TEMP_GREEN` | Ambient 26.0 C to below 30.0 C    |
| `PB1`  | `LED_TEMP_BLUE`  | Ambient below 26.0 C              |

All LEDs are active high.

#### Tasks

| Task           | Priority     | Module                        | Job                                                                      |
| -------------- | ------------ | ----------------------------- | ------------------------------------------------------------------------ |
| `defaultTask`  | Normal       | `app_uart`, `app_cmd`         | Read received bytes from a queue and run the command parser.             |
| `blinkTask`    | Normal       | `app_led`                     | Own the two blink LEDs; take requests from a queue.                      |
| `sensorTask`   | Normal       | `app_sensor`                  | Process ADC samples (thread flag from the DMA callback), moving average. |
| `mcp9808Task`  | Normal       | `app_mcp9808`, `app_temp_led` | Read the MCP9808 once per second and drive the temperature LEDs.         |
| `watchdogTask` | Above normal | `app_watchdog`                | Check that every task has checked in, then refresh the IWDG.             |

Other modules: `app_config` (Flash Sector 7 record) and `app_reset`
(reset cause).

#### Commands

One per line, LF or CRLF, lowercase.

| Command              | Description                                                                                  |
| -------------------- | -------------------------------------------------------------------------------------------- |
| `help`               | List available commands.                                                                     |
| `led on` / `led off` | Turn both blink LEDs on or off.                                                              |
| `blink <100-2000>`   | Alternate the two blink LEDs with the given period in ms.                                    |
| `blink off`          | Stop blinking and hold the current LED state.                                                |
| `status`             | Show the LED mode and the last reset cause.                                                  |
| `stats`              | Show uptime (s), RX events, RX dropped bytes, and TX dropped messages.                       |
| `reset stats`        | Reset the diagnostic counters.                                                               |
| `adc`                | Show raw value, chip temperature, moving average, samples, processed count, and timeouts.    |
| `sample <100-10000>` | Set the ADC sampling period in ms.                                                           |
| `temp`               | Show chip temperature (average), ambient temperature, LED zone, and MCP9808 OK/error counts. |
| `config show`        | Show the current configuration and the one stored in Flash (or `none`).                      |
| `config save`        | Save LED mode, blink period, and sampling period to Flash.                                   |
| `config reset`       | Erase the stored configuration.                                                              |
| `wdg spin`           | Debug build only. Make `defaultTask` spin forever to test the watchdog.                      |
| `wdg block`          | Debug build only. Suspend `defaultTask` to test the watchdog.                                |

#### Design notes

- Module rules: each `app_*.h` holds only the public API; everything else in
  the `.c` file is `static`. Peripheral handles are passed in through `init`.
  Each resource has exactly one owner, and the limits for a resource are
  defined in its owner's header.
- Between tasks, requests travel through queues (for example LED mode
  changes), and shared data is published as a mutex-protected snapshot that
  readers copy (`app_sensor_read`, `app_mcp9808_read`).
- The temperature LEDs are written only by `mcp9808Task`, so they need no
  mutex. All three are on GPIOB and are updated with a single `BSRR` write.
- Temperature zones use integer thresholds in 0.1 degree C (`300`, `260`)
  with 0.3 degree C hysteresis: moving up uses the threshold, moving down
  requires going below threshold minus 0.3 (for example red to green below
  29.7 C). If the MCP9808 read fails, all three LEDs turn off. The chip
  temperature is not used as a fallback because it reads several degrees high.
- At boot the configuration is read from Flash, or the defaults are used
  (LED off, blink 500 ms, sampling 100 ms). Both paths then go through the
  same apply step, and a value that the module rejects is replaced by the
  default. The configuration record is version 2 (the LED mode enum order
  changed, so version 1 records are rejected).
- The reset cause is read from `RCC->CSR` once at boot, then the flags are
  cleared. Specific causes are checked first (IWDG, WWDG, software, low
  power, power-on/brown-out) and the pin reset last, because every internal
  reset also pulls NRST low and sets the pin flag.

#### Watchdog

- IWDG prescaler 64, reload 2499: about 5 s at the nominal 32 kHz LSI, but
  between about 3.4 s and 9.4 s across the LSI tolerance, so designs assume
  3.4 s.
- `watchdogTask` runs every 500 ms. Every other task calls
  `app_watchdog_checkin()` at the top of its loop. The IWDG is refreshed only
  if every task checked in within the last 3000 ms. Otherwise the watchdog
  prints `WDG: <task> timeout` once and stops refreshing, and the IWDG resets
  the MCU.
- Only `app_watchdog` calls `HAL_IWDG_Refresh`, so a stuck task cannot be
  hidden by someone else refreshing the watchdog.
- To make check-ins possible, no task waits forever: UART reads wait up to
  1000 ms, the LED task waits up to 1000 ms when not blinking, and the sensor
  task waits in 1000 ms slices. A sensor timeout is counted only when no
  sample has arrived for twice the sampling period.
- `__HAL_DBGMCU_FREEZE_IWDG()` stops the IWDG while the debugger halts the
  CPU, so breakpoints do not cause resets.
- A Flash erase pauses the CPU for about 1 to 2 s, which fits inside the
  watchdog margin.

#### Bugs found and fixed

- **Chip temperature jumped to about 1,745,951 C.** The calibration
  temperature constants were written as `30u` and `110u`. One unsigned
  operand makes the whole expression unsigned, so when the chip first cooled
  below 30 C (raw value below the 30 C calibration value), the negative
  difference wrapped around. The bug existed from the start but only showed
  once the weather got colder. Fix: signed constants. Lesson: test both
  sides of every boundary.
- **`status` showed LED ON while the LEDs were off.** The LED enum started
  with `APP_LED_ON`, so the zero-initialized status variable meant ON, and
  the defaults path at boot never sent a request to the LED task. Fix:
  `APP_LED_OFF` is now the first enum value, the status variables are
  initialized explicitly, both boot paths apply the configuration, and every
  apply call checks its return value.

#### Known limitations

- No I2C bus recovery (manual SCL clocking when a slave holds SDA low).
  Shorting SDA to GND for a few seconds was tested and the HAL recovered on
  its own, so it was left out.
- Integer division truncates toward zero, so the chip temperature below
  30 C can read up to 0.1 C high.

## Repository Notes

Committed: `.ioc`, `Core/`, `Drivers/`, `*.ld`, `.project`, `.cproject`, `.settings/`.

Ignored build and workspace output: `**/Debug/`, `**/Release/`, `.metadata/`,
`RemoteSystemsTempFiles/`, `*.exe`.

After regenerating code with CubeMX, re-check the IRQ and DMA handlers and the
`FLASH` length in the linker script.

In `nucleo_f446re_sensor_monitor`, the preprocessor symbol
`APP_DEBUG_COMMANDS` is defined only in the Debug build configuration, so the
watchdog test commands do not exist in Release builds.
