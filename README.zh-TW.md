# STM32 韌體學習

[English](README.md) | 繁體中文

STM32 韌體開發的實作學習 repository。

## 目標

使用以下技術完成一個 STM32 感測器監控系統：

- C
- STM32CubeIDE
- GPIO、UART、Timer、ADC、I2C
- FreeRTOS task 與 queue
- 錯誤處理與 watchdog 恢復

## 硬體與工具

- 開發板：NUCLEO-F446RE（MCU：STM32F446RE）
- STM32CubeMX：周邊設定與程式碼產生
- STM32CubeIDE：編譯、燒錄、除錯
- 除錯器／燒錄器：板載 ST-LINK（`CN1` Mini-USB）
- 序列埠終端：使用 `System.IO.Ports.SerialPort` 的 PowerShell 腳本
  （ST-LINK Virtual COM Port、USART2、`115200`、8-N-1、無流量控制）
- 外接元件（後期專案）：麵包板 LED x5、MCP9808 溫度感測模組

## 進度

- [x] C 位元運算（暫存器層級 GPIO 專案）
- [x] 環形緩衝區（UART DMA 專案中的 RX 與 TX ring buffer）
- [x] UART 記錄輸出（中斷式與 DMA 式指令介面）
- [x] 計時器觸發的 ADC 取樣搭配 DMA（內建溫度感測器）
- [x] I2C 感測器（MCP9808）
- [x] FreeRTOS（task、queue、thread flags、mutex）
- [x] 最終的感測器監控專案

另外完成：把設定儲存在內部 Flash（`CONFIG SHOW`、`CONFIG SAVE`、`CONFIG RESET`），
包含 blink 週期與 ADC 取樣週期；以及在感測器監控專案中加入獨立看門狗（IWDG）
與逐一 task 的監督機制。

## 專案

所有專案都在 `projects/` 底下。

| 專案                              | 說明                                                                                                                              |
| --------------------------------- | --------------------------------------------------------------------------------------------------------------------------------- |
| `nucleo_f446re_blink`             | 讓使用者 LED（`LD2`、`PA5`）閃爍。                                                                                                |
| `nucleo_f446re_button_blink`      | 按下使用者按鈕（`B1`、`PC13`、EXTI）切換慢閃與快閃。                                                                              |
| `nucleo_f446re_uart_commands`     | 透過 Virtual COM Port 的文字指令控制器，使用 UART 接收中斷（一次一個 byte）。                                                     |
| `nucleo_f446re_uart_dma_commands` | 以 circular RX DMA 與排隊式 TX DMA 建構的 UART 指令控制器，含非阻塞 blink 排程、診斷資訊與 Flash 設定。                           |
| `nucleo_f446re_adc_sampling`      | 複製 UART DMA 專案，加上內建溫度感測器的週期性 ADC 取樣（TIM2 觸發、ADC1、DMA）、`ADC` 指令，以及可存入 Flash 的取樣週期。        |
| `nucleo_f446re_register_gpio`     | 不使用 HAL 的 GPIO：直接寫入 `MODER`、`PUPDR`、`BSRR`，讀取 `IDR` 來設定與控制腳位。                                              |
| `nucleo_f446re_freertos`          | FreeRTOS（CMSIS-RTOS v2）基礎：task、message queue、thread flags、mutex。                                                         |
| `nucleo_f446re_i2c_mcp9808`       | 透過 I2C1 讀取 MCP9808 的環境溫度，並轉換 13 位元二補數數值。                                                                     |
| `nucleo_f446re_sensor_monitor`    | **目前的主專案。** 以 FreeRTOS 模組化重寫，整合 UART 指令、LED、ADC 晶片溫度、MCP9808 環境溫度、溫度指示 LED、Flash 設定與 IWDG。 |

### `nucleo_f446re_uart_dma_commands`

資料路徑：

```text
USART2 RX DMA（circular）
-> DMA RX 緩衝區
-> 軟體 RX ring buffer
-> 主迴圈的指令解析器
-> 軟體 TX ring buffer
-> USART2 TX DMA
-> TX 完成 callback 啟動下一個排隊中的區塊
```

指令（一行一個，LF 或 CRLF，區分大小寫）：

| 指令                 | 說明                                                                  |
| -------------------- | --------------------------------------------------------------------- |
| `HELP`               | 列出可用指令。                                                        |
| `LED ON` / `LED OFF` | 設定 LED 並關閉 blink 模式。                                          |
| `STATUS`             | 顯示 LED 狀態與 blink 狀態。                                          |
| `BLINK <100-2000>`   | 以指定週期（ms）閃爍 LED（非阻塞）。                                  |
| `BLINK OFF`          | 停止閃爍並保持目前的 LED 狀態。                                       |
| `STATS`              | 顯示運行時間、RX 事件數、RX 丟棄 byte 數、過長指令數、TX 丟棄訊息數。 |
| `RESET STATS`        | 重設診斷計數器（運行時間不重設）。                                    |
| `CONFIG SHOW`        | 顯示目前的 blink 週期與 Flash 中儲存的值。                            |
| `CONFIG SAVE`        | 將目前的 blink 週期存入 Flash（抹除、寫入、讀回、驗證）。             |
| `CONFIG RESET`       | 抹除已儲存的設定，回到預設週期（500 ms）。                            |

設計重點：

- callback 只負責搬移資料與更新計數器，指令解析與其他較耗時的工作都在主迴圈執行。
- TX ring buffer 滿的時候，一則回覆要嘛完整排入，要嘛整則丟棄，並計算丟棄次數。
- 設定存放在 Flash Sector 7（`0x08060000`），包含 magic number、版本與 checksum。
  資料無效或空白時使用預設週期（500 ms）。linker script 的 `FLASH` 長度限制為
  `384K`，確保程式碼永遠不會覆蓋到 Sector 7。
- 儲存與重設都會抹除整個 128 KB 的 sector，因此執行 `CONFIG SAVE` 或
  `CONFIG RESET` 時 CPU 會暫停約一秒以上。`CONFIG RESET` 只做抹除，因為空白的
  sector 本身就代表「沒有儲存的設定」。

### `nucleo_f446re_adc_sampling`

以 UART DMA 專案為基礎，所以原本的所有指令與 Flash 設定都還能使用。這個專案加上
內建溫度感測器的週期性取樣。

取樣路徑：

```text
TIM2 update 事件（預設 10 Hz，由 SAMPLE 設定）
-> TRGO 觸發一次 ADC1 轉換（溫度感測器通道）
-> ADC1 DMA（circular、half-word）把結果寫入 RAM
-> 轉換完成 callback 增加取樣計數
-> 主迴圈在需要時讀取最新值
```

新增與修改的指令：

| 指令                 | 說明                                                        |
| -------------------- | ----------------------------------------------------------- |
| `ADC`                | 顯示最新的 ADC 原始值、攝氏溫度，以及開機以來的取樣次數。   |
| `SAMPLE <100-10000>` | 設定取樣週期（ms），立即生效。                              |
| `CONFIG SHOW`        | 顯示 blink 週期與取樣週期的目前值與儲存值。                 |
| `CONFIG SAVE`        | 將兩個週期都存入 Flash。                                    |
| `CONFIG RESET`       | 抹除已儲存的設定，回到預設值（blink 500 ms、取樣 100 ms）。 |

設計重點：

- 取樣時間由硬體計時器決定，所以不受主迴圈忙碌程度影響。TIM2 的計時器時脈為
  84 MHz，prescaler 為 8399，計數頻率為 10 kHz（每 ms 10 個 tick）。預設週期
  999 對應 10 Hz。
- `SAMPLE` 把 auto-reload 暫存器設為 `ms * 10 - 1`，然後把計數器歸零。
  auto-reload preload 是關閉的，若不歸零，計數器可能已經超過新的較小上限，
  就會一路數到 32 位元計數器繞回，取樣會停止好幾天。
- 設定格式升為版本 2，並多存取樣週期。Flash 中的版本 1 資料會驗證失敗而改用
  預設值，這是預期的升級行為。開機時儲存的週期會在 timer 啟動前套用到 TIM2，
  `CONFIG RESET` 會立刻把預設值套用到 TIM2。
- ADC 取樣時間為 21 MHz ADC 時脈下的 480 cycles（約 23 us），比 datasheet 對
  溫度感測器要求的最小值還長。
- 溫度以 0.1 °C 為單位的整數運算計算，使用 MCU 內儲存的原廠校正值（兩點線性內插）。
  不使用浮點數，因為 newlib-nano 預設關閉 `printf` 的浮點支援。
- 輸出文字為純 ASCII。非 ASCII 的度數符號在序列埠終端會顯示成 `??`。
- 內建感測器量的是晶片溫度，通常比室溫高幾度。這裡用來學習取樣流程，不作為最終
  的感測器。
- 執行 `CONFIG SAVE` 與 `CONFIG RESET` 時，取樣計數可能會稍微落後，因為 Flash
  抹除期間 CPU 暫停，轉換完成 callback 會延後執行。DMA 本身持續運作。

### `nucleo_f446re_register_gpio`

暫存器層級 GPIO 存取的練習專案，不使用 HAL 的 GPIO 函式。

- `MODER` 每個腳位佔 2 個位元，所以腳位的欄位在第 `2 * pin` 位元。
- `PUPDR` 設定輸入腳位的上拉或下拉。
- `BSRR` 的低 16 位元設定腳位、高 16 位元清除腳位，一次原子寫入完成
  （不需要讀取、修改、寫回）。
- 讀取 `IDR` 取得輸入電位。

### `nucleo_f446re_freertos`

透過 CMSIS-RTOS v2 API 練習 FreeRTOS：建立 task、用 message queue 傳遞資料、
用 thread flags 從中斷喚醒 task，以及用 mutex 保護共享資料。

### `nucleo_f446re_i2c_mcp9808`

讀取 MCP9808 環境溫度暫存器的獨立 I2C 專案。

接線：

| MCP9808 | NUCLEO-F446RE     |
| ------- | ----------------- |
| VIN     | 3V3               |
| GND     | GND               |
| SCL     | `PB8`（`CN10-3`） |
| SDA     | `PB9`（`CN10-5`） |

- I2C1 速度 100 kHz，7 位元位址 `0x18`，暫存器 `0x05`，讀取 2 bytes。
- 數值為 13 位元二補數，單位 1/16 °C：先用 `0x1FFF` 遮罩，若第 12 位元為 1 則減去
  `0x2000`，再用 `* 10 / 16` 換算成 0.1 °C 單位。

### `nucleo_f446re_sensor_monitor`

最終專案。以 FreeRTOS 模組化重寫，整合以上所有內容：DMA 式 UART 指令、交替閃爍
的 LED、ADC 晶片溫度、MCP9808 環境溫度、紅綠藍溫度指示 LED、Flash 設定、重置原因
回報，以及具備逐一 task 監督的獨立看門狗。

接線（MCP9808 同上）：

| 腳位   | 標籤             | 用途                            |
| ------ | ---------------- | ------------------------------- |
| `PB12` | `LED_BLINK_A`    | 閃爍 LED A                      |
| `PB13` | `LED_BLINK_B`    | 閃爍 LED B（與 A 相位相反）     |
| `PB14` | `LED_TEMP_RED`   | 環境溫度 >= 30.0 °C             |
| `PB15` | `LED_TEMP_GREEN` | 環境溫度 26.0 °C 至未滿 30.0 °C |
| `PB1`  | `LED_TEMP_BLUE`  | 環境溫度未滿 26.0 °C            |

所有 LED 皆為高電位點亮。

#### Task

| Task           | 優先權       | 模組                          | 工作                                                              |
| -------------- | ------------ | ----------------------------- | ----------------------------------------------------------------- |
| `defaultTask`  | Normal       | `app_uart`、`app_cmd`         | 從 queue 讀取收到的 byte，執行指令解析。                          |
| `blinkTask`    | Normal       | `app_led`                     | 擁有兩顆閃爍 LED，從 queue 接收請求。                             |
| `sensorTask`   | Normal       | `app_sensor`                  | 處理 ADC 樣本（由 DMA callback 設定 thread flag），計算移動平均。 |
| `mcp9808Task`  | Normal       | `app_mcp9808`、`app_temp_led` | 每秒讀取一次 MCP9808，並控制溫度 LED。                            |
| `watchdogTask` | Above normal | `app_watchdog`                | 確認所有 task 都有報到後，才餵 IWDG。                             |

其他模組：`app_config`（Flash Sector 7 的設定紀錄）與 `app_reset`（重置原因）。

#### 指令

一行一個，LF 或 CRLF，小寫。

| 指令                 | 說明                                                                    |
| -------------------- | ----------------------------------------------------------------------- |
| `help`               | 列出可用指令。                                                          |
| `led on` / `led off` | 兩顆閃爍 LED 同時亮或滅。                                               |
| `blink <100-2000>`   | 以指定週期（ms）交替閃爍兩顆 LED。                                      |
| `blink off`          | 停止閃爍並保持目前的 LED 狀態。                                         |
| `status`             | 顯示 LED 模式與上次的重置原因。                                         |
| `stats`              | 顯示運行時間（秒）、RX 事件數、RX 丟棄 byte 數、TX 丟棄訊息數。         |
| `reset stats`        | 重設診斷計數器。                                                        |
| `adc`                | 顯示原始值、晶片溫度、移動平均、取樣數、處理數與逾時次數。              |
| `sample <100-10000>` | 設定 ADC 取樣週期（ms）。                                               |
| `temp`               | 顯示晶片溫度（平均）、環境溫度、LED 區間，以及 MCP9808 成功／失敗次數。 |
| `config show`        | 顯示目前設定與 Flash 中儲存的設定（或 `none`）。                        |
| `config save`        | 將 LED 模式、blink 週期與取樣週期存入 Flash。                           |
| `config reset`       | 抹除已儲存的設定。                                                      |
| `wdg spin`           | 僅限 Debug 版本。讓 `defaultTask` 進入無窮迴圈，測試 watchdog。         |
| `wdg block`          | 僅限 Debug 版本。暫停 `defaultTask`，測試 watchdog。                    |

#### 設計重點

- 模組規則：每個 `app_*.h` 只放公開 API，`.c` 內其餘內容全部 `static`。
  周邊 handle 透過 `init` 傳入。每個資源只有一個擁有者，資源的上下限定義在
  擁有者的 header。
- task 之間用 queue 傳遞請求（例如 LED 模式變更），共享資料以 mutex 保護的
  snapshot 發布，讀取端取得一份複本（`app_sensor_read`、`app_mcp9808_read`）。
- 溫度 LED 只由 `mcp9808Task` 寫入，所以不需要 mutex。三顆都在 GPIOB，
  用一次 `BSRR` 寫入同時更新。
- 溫度區間使用 0.1 °C 單位的整數門檻（`300`、`260`），遲滯 0.3 °C：往上用原門檻，
  往下要低於「門檻減 0.3」才切換（例如紅轉綠要低於 29.7 °C）。MCP9808 讀取失敗
  時三顆 LED 全滅。不使用晶片溫度作為備援，因為它會偏高好幾度。
- 開機時從 Flash 讀取設定，或使用預設值（LED 關、blink 500 ms、取樣 100 ms）。
  兩條路徑都經過同一個套用步驟，被模組拒絕的值會改用預設值。設定紀錄為版本 2
  （LED 模式 enum 的順序改變，所以版本 1 的紀錄會被拒絕）。
- 重置原因在開機時從 `RCC->CSR` 讀取一次，之後清除旗標。先檢查具體的原因
  （IWDG、WWDG、軟體、低功耗、上電／欠壓），最後才檢查 pin reset，因為所有內部
  重置都會把 NRST 拉低並設定 pin 旗標。

#### Watchdog

- IWDG prescaler 64、reload 2499：LSI 為標稱 32 kHz 時約 5 秒，但依 LSI 誤差
  範圍約在 3.4 秒到 9.4 秒之間，所以設計以 3.4 秒為準。
- `watchdogTask` 每 500 ms 執行一次。其他每個 task 都在迴圈開頭呼叫
  `app_watchdog_checkin()`。只有所有 task 都在最近 3000 ms 內報到過，才會餵
  IWDG。否則 watchdog 印出一次 `WDG: <task> timeout` 並停止餵狗，由 IWDG 重置 MCU。
- 只有 `app_watchdog` 會呼叫 `HAL_IWDG_Refresh`，避免卡住的 task 因為別處餵狗
  而被掩蓋。
- 為了讓每個 task 都能報到，沒有任何 task 會無限等待：UART 讀取最多等 1000 ms，
  LED task 在非閃爍模式最多等 1000 ms，sensor task 以 1000 ms 為單位分段等待。
  只有超過兩倍取樣週期都沒收到樣本，才計為一次 sensor 逾時。
- `__HAL_DBGMCU_FREEZE_IWDG()` 讓除錯器暫停 CPU 時 IWDG 也一起暫停，中斷點不會
  造成重置。
- Flash 抹除會讓 CPU 暫停約 1 到 2 秒，仍在 watchdog 的餘裕之內。

#### 發現並修正的問題

- **晶片溫度跳到約 1,745,951 °C。** 校正溫度常數寫成 `30u` 與 `110u`。只要有一個
  無號運算元，整條運算式就會以無號數計算，所以當晶片第一次降到 30 °C 以下
  （原始值低於 30 °C 校正值）時，負的差值發生繞回。這個 bug 從一開始就存在，
  直到天氣變冷才出現。修正：改用有號常數。教訓：每個邊界的兩側都要測試。
- **`status` 顯示 LED ON，但 LED 實際是滅的。** LED enum 的第一個值是
  `APP_LED_ON`，所以被清成 0 的狀態變數代表 ON，而開機的預設值路徑從來沒有送出
  請求給 LED task。修正：`APP_LED_OFF` 改為 enum 第一個值、狀態變數明確初始化、
  兩條開機路徑都會套用設定，且每次套用都檢查回傳值。

#### 已知限制

- 沒有實作 I2C bus recovery（slave 拉住 SDA 時手動產生 SCL 時脈）。測試過把 SDA
  短接到 GND 數秒，HAL 能自行恢復，所以沒有實作。
- 整數除法向零截斷，所以晶片溫度在 30 °C 以下時可能偏高最多 0.1 °C。

## Repository 說明

納入版本控制：`.ioc`、`Core/`、`Drivers/`、`*.ld`、`.project`、`.cproject`、`.settings/`。

忽略的編譯與工作區產出：`**/Debug/`、`**/Release/`、`.metadata/`、
`RemoteSystemsTempFiles/`、`*.exe`。

用 CubeMX 重新產生程式碼後，要重新檢查 IRQ 與 DMA handler，以及 linker script 的
`FLASH` 長度。

在 `nucleo_f446re_sensor_monitor` 中，前置處理器符號 `APP_DEBUG_COMMANDS` 只定義在
Debug 編譯設定，所以 Release 版本不包含 watchdog 測試指令。
