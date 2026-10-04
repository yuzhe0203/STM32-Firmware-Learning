/* USER CODE BEGIN Header */
/**
  ******************************************************************************
  * @file           : main.c
  * @brief          : Main program body
  ******************************************************************************
  * @attention
  *
  * Copyright (c) 2026 STMicroelectronics.
  * All rights reserved.
  *
  * This software is licensed under terms that can be found in the LICENSE file
  * in the root directory of this software component.
  * If no LICENSE file comes with this software, it is provided AS-IS.
  *
  ******************************************************************************
  */
/* USER CODE END Header */
/* Includes ------------------------------------------------------------------*/
#include "main.h"

/* Private includes ----------------------------------------------------------*/
/* USER CODE BEGIN Includes */

#include <stdbool.h>
#include <string.h>
#include "stm32f4xx_nucleo.h"
#include <stdio.h>
#include <stdlib.h>

/* USER CODE END Includes */

/* Private typedef -----------------------------------------------------------*/
/* USER CODE BEGIN PTD */

typedef struct {
	uint32_t magic;
	uint32_t version;
	uint32_t blink_period_ms;
	uint32_t checksum;
} app_config_t;

/* USER CODE END PTD */

/* Private define ------------------------------------------------------------*/
/* USER CODE BEGIN PD */

#define CONFIG_FLASH_ADDRESS 0x08060000u
#define CONFIG_FLASH_SECTOR FLASH_SECTOR_7
#define CONFIG_MAGIC 0x20040203u
#define CONFIG_VERSION 1u
#define BLINK_DEFAULT_MS 500u

#define ADC_DMA_BUFFER_LENGTH 1u

/* USER CODE END PD */

/* Private macro -------------------------------------------------------------*/

/* USER CODE BEGIN PM */

/* USER CODE END PM */

/* Private variables ---------------------------------------------------------*/

ADC_HandleTypeDef hadc1;
DMA_HandleTypeDef hdma_adc1;

TIM_HandleTypeDef htim2;

UART_HandleTypeDef huart2;
DMA_HandleTypeDef hdma_usart2_rx;
DMA_HandleTypeDef hdma_usart2_tx;

/* USER CODE BEGIN PV */

#define UART_DMA_RX_CAPACITY 64u
#define UART_RING_CAPACITY 128u
#define UART_COMMAND_CAPACITY 32u

static uint8_t uart_dma_rx[UART_DMA_RX_CAPACITY];
static volatile uint16_t uart_dma_last_position = 0u;

/*
 *
 * DMA callback is the producer; the main loop is the consumer.
 * One slot remains unused so head == tail unambiguously means empty.
 */
static uint8_t uart_ring[UART_RING_CAPACITY];
static volatile uint16_t uart_ring_head = 0u;
static volatile uint16_t uart_ring_tail = 0u;
static volatile uint8_t uart_ring_overflow = 0u;

static char uart_command[UART_COMMAND_CAPACITY];
static uint16_t uart_command_len = 0u;
static uint8_t uart_discard_command = 0u;
static uint8_t uart_last_was_cr = 0u;

static bool led_is_on = false;

#define UART_TX_RING_CAPACITY 256u

/*
 *
 * Main loop produces response text.
 * UART TX-complete callback consumes transmitted bytes.
 *
 * One byte remains unused so head == tail always means empty.
 */
static uint8_t uart_tx_ring[UART_TX_RING_CAPACITY];
static volatile uint16_t uart_tx_head = 0u;
static volatile uint16_t uart_tx_tail = 0u;

static volatile uint16_t uart_tx_dma_len = 0u;
static volatile uint8_t uart_tx_dma_active = 0u;

/*
 *
 * Overflow policy
 *  - Do not partially enqueue a message.
 *  - Drop the newest entire message if there is insufficient space.
 *  - Count Dropped messages for debugging.
 */
static volatile uint32_t uart_tx_dropped_messages = 0u;
static volatile uint32_t uart_rx_events = 0u;
static volatile uint32_t uart_rx_dropped_byte = 0u;
static volatile uint32_t uart_rx_command_overflows = 0u;

#define BLINK_MIN_PERIOD_MS 100u
#define BLINK_MAX_PERIOD_MS 2000u

static bool blink_enable = false;
static uint32_t blink_period_ms = BLINK_DEFAULT_MS;
static uint32_t last_blink_ms = 0u;

static bool config_loaded_from_flash = false;

static volatile uint16_t adc_dma_buffer[] = {0};
static volatile uint32_t adc_sample_count = 0u;

/* USER CODE END PV */

/* Private function prototypes -----------------------------------------------*/
void SystemClock_Config(void);
static void MX_GPIO_Init(void);
static void MX_DMA_Init(void);
static void MX_USART2_UART_Init(void);
static void MX_ADC1_Init(void);
static void MX_TIM2_Init(void);
/* USER CODE BEGIN PFP */

static void uart_send_text(const char *text);
static void set_led(bool on);
static void process_command(const char *command);
static void process_uart_byte(uint8_t byte);

static void uart_ring_push(uint8_t byte);
static bool uart_ring_pop(uint8_t *byte);

static void uart_tx_kick_locked(void);
static uint16_t uart_tx_free_space_locked(void);

static void run_blink_task(void);
static void send_status(void);

static void send_stats(void);
static void reset_stats(void);

static uint32_t config_calculate_checksum(const app_config_t *config);
static bool config_is_valid(const app_config_t *config);
static void config_load(void);
static void send_config(void);
static bool config_save(void);
static bool config_erase(void);

static void send_adc(void);

/* USER CODE END PFP */

/* Private user code ---------------------------------------------------------*/
/* USER CODE BEGIN 0 */

/* USER CODE END 0 */

/**
  * @brief  The application entry point.
  * @retval int
  */
int main(void)
{

  /* USER CODE BEGIN 1 */

  /* USER CODE END 1 */

  /* MCU Configuration--------------------------------------------------------*/

  /* Reset of all peripherals, Initializes the Flash interface and the Systick. */
  HAL_Init();

  /* USER CODE BEGIN Init */

  /* USER CODE END Init */

  /* Configure the system clock */
  SystemClock_Config();

  /* USER CODE BEGIN SysInit */

  /* USER CODE END SysInit */

  /* Initialize all configured peripherals */
  MX_GPIO_Init();
  MX_DMA_Init();
  MX_USART2_UART_Init();
  MX_ADC1_Init();
  MX_TIM2_Init();
  /* USER CODE BEGIN 2 */

  set_led(false);
  config_load();

  if (HAL_UARTEx_ReceiveToIdle_DMA(&huart2, uart_dma_rx, UART_DMA_RX_CAPACITY) != HAL_OK)
	  Error_Handler();

  if (HAL_ADC_Start_DMA(&hadc1, (uint32_t *)adc_dma_buffer, ADC_DMA_BUFFER_LENGTH) != HAL_OK)
	  Error_Handler();

  if (HAL_TIM_Base_Start(&htim2) != HAL_OK)
	  Error_Handler();

  last_blink_ms = HAL_GetTick();

  uart_send_text("\r\nUART DMA command controller ready\r\n");
  uart_send_text("Commands Details, Type 'HELP'\r\n");
  uart_send_text(config_loaded_from_flash ? "Config: loaded from flash\r\n> " : "Config: using defaults\r\n> ");

  /* USER CODE END 2 */

  /* Initialize leds */
  BSP_LED_Init(LED2);

  /* Infinite loop */
  /* USER CODE BEGIN WHILE */
  while (1)
  {

    /* USER CODE END WHILE */

    /* USER CODE BEGIN 3 */

	  uint8_t byte;

	  run_blink_task();

	  if (uart_ring_overflow != 0u) {
		  uart_ring_overflow = 0u;
		  uart_command_len = 0u;
		  uart_discard_command = 1u;
		  uart_send_text("ERROR: receive buffer overflow\r\n> ");
	  }

	  while (uart_ring_pop(&byte)) {
		  process_uart_byte(byte);
	  }
  }
  /* USER CODE END 3 */
}

/**
  * @brief System Clock Configuration
  * @retval None
  */
void SystemClock_Config(void)
{
  RCC_OscInitTypeDef RCC_OscInitStruct = {0};
  RCC_ClkInitTypeDef RCC_ClkInitStruct = {0};

  /** Configure the main internal regulator output voltage
  */
  __HAL_RCC_PWR_CLK_ENABLE();
  __HAL_PWR_VOLTAGESCALING_CONFIG(PWR_REGULATOR_VOLTAGE_SCALE3);

  /** Initializes the RCC Oscillators according to the specified parameters
  * in the RCC_OscInitTypeDef structure.
  */
  RCC_OscInitStruct.OscillatorType = RCC_OSCILLATORTYPE_HSI;
  RCC_OscInitStruct.HSIState = RCC_HSI_ON;
  RCC_OscInitStruct.HSICalibrationValue = RCC_HSICALIBRATION_DEFAULT;
  RCC_OscInitStruct.PLL.PLLState = RCC_PLL_ON;
  RCC_OscInitStruct.PLL.PLLSource = RCC_PLLSOURCE_HSI;
  RCC_OscInitStruct.PLL.PLLM = 16;
  RCC_OscInitStruct.PLL.PLLN = 336;
  RCC_OscInitStruct.PLL.PLLP = RCC_PLLP_DIV4;
  RCC_OscInitStruct.PLL.PLLQ = 2;
  RCC_OscInitStruct.PLL.PLLR = 2;
  if (HAL_RCC_OscConfig(&RCC_OscInitStruct) != HAL_OK)
  {
    Error_Handler();
  }

  /** Initializes the CPU, AHB and APB buses clocks
  */
  RCC_ClkInitStruct.ClockType = RCC_CLOCKTYPE_HCLK|RCC_CLOCKTYPE_SYSCLK
                              |RCC_CLOCKTYPE_PCLK1|RCC_CLOCKTYPE_PCLK2;
  RCC_ClkInitStruct.SYSCLKSource = RCC_SYSCLKSOURCE_PLLCLK;
  RCC_ClkInitStruct.AHBCLKDivider = RCC_SYSCLK_DIV1;
  RCC_ClkInitStruct.APB1CLKDivider = RCC_HCLK_DIV2;
  RCC_ClkInitStruct.APB2CLKDivider = RCC_HCLK_DIV1;

  if (HAL_RCC_ClockConfig(&RCC_ClkInitStruct, FLASH_LATENCY_2) != HAL_OK)
  {
    Error_Handler();
  }
}

/**
  * @brief ADC1 Initialization Function
  * @param None
  * @retval None
  */
static void MX_ADC1_Init(void)
{

  /* USER CODE BEGIN ADC1_Init 0 */

  /* USER CODE END ADC1_Init 0 */

  ADC_ChannelConfTypeDef sConfig = {0};

  /* USER CODE BEGIN ADC1_Init 1 */

  /* USER CODE END ADC1_Init 1 */

  /** Configure the global features of the ADC (Clock, Resolution, Data Alignment and number of conversion)
  */
  hadc1.Instance = ADC1;
  hadc1.Init.ClockPrescaler = ADC_CLOCK_SYNC_PCLK_DIV4;
  hadc1.Init.Resolution = ADC_RESOLUTION_12B;
  hadc1.Init.ScanConvMode = DISABLE;
  hadc1.Init.ContinuousConvMode = DISABLE;
  hadc1.Init.DiscontinuousConvMode = DISABLE;
  hadc1.Init.ExternalTrigConvEdge = ADC_EXTERNALTRIGCONVEDGE_RISING;
  hadc1.Init.ExternalTrigConv = ADC_EXTERNALTRIGCONV_T2_TRGO;
  hadc1.Init.DataAlign = ADC_DATAALIGN_RIGHT;
  hadc1.Init.NbrOfConversion = 1;
  hadc1.Init.DMAContinuousRequests = ENABLE;
  hadc1.Init.EOCSelection = ADC_EOC_SINGLE_CONV;
  if (HAL_ADC_Init(&hadc1) != HAL_OK)
  {
    Error_Handler();
  }

  /** Configure for the selected ADC regular channel its corresponding rank in the sequencer and its sample time.
  */
  sConfig.Channel = ADC_CHANNEL_TEMPSENSOR;
  sConfig.Rank = 1;
  sConfig.SamplingTime = ADC_SAMPLETIME_480CYCLES;
  if (HAL_ADC_ConfigChannel(&hadc1, &sConfig) != HAL_OK)
  {
    Error_Handler();
  }
  /* USER CODE BEGIN ADC1_Init 2 */

  /* USER CODE END ADC1_Init 2 */

}

/**
  * @brief TIM2 Initialization Function
  * @param None
  * @retval None
  */
static void MX_TIM2_Init(void)
{

  /* USER CODE BEGIN TIM2_Init 0 */

  /* USER CODE END TIM2_Init 0 */

  TIM_ClockConfigTypeDef sClockSourceConfig = {0};
  TIM_MasterConfigTypeDef sMasterConfig = {0};

  /* USER CODE BEGIN TIM2_Init 1 */

  /* USER CODE END TIM2_Init 1 */
  htim2.Instance = TIM2;
  htim2.Init.Prescaler = 8399;
  htim2.Init.CounterMode = TIM_COUNTERMODE_UP;
  htim2.Init.Period = 999;
  htim2.Init.ClockDivision = TIM_CLOCKDIVISION_DIV1;
  htim2.Init.AutoReloadPreload = TIM_AUTORELOAD_PRELOAD_DISABLE;
  if (HAL_TIM_Base_Init(&htim2) != HAL_OK)
  {
    Error_Handler();
  }
  sClockSourceConfig.ClockSource = TIM_CLOCKSOURCE_INTERNAL;
  if (HAL_TIM_ConfigClockSource(&htim2, &sClockSourceConfig) != HAL_OK)
  {
    Error_Handler();
  }
  sMasterConfig.MasterOutputTrigger = TIM_TRGO_UPDATE;
  sMasterConfig.MasterSlaveMode = TIM_MASTERSLAVEMODE_DISABLE;
  if (HAL_TIMEx_MasterConfigSynchronization(&htim2, &sMasterConfig) != HAL_OK)
  {
    Error_Handler();
  }
  /* USER CODE BEGIN TIM2_Init 2 */

  /* USER CODE END TIM2_Init 2 */

}

/**
  * @brief USART2 Initialization Function
  * @param None
  * @retval None
  */
static void MX_USART2_UART_Init(void)
{

  /* USER CODE BEGIN USART2_Init 0 */

  /* USER CODE END USART2_Init 0 */

  /* USER CODE BEGIN USART2_Init 1 */

  /* USER CODE END USART2_Init 1 */
  huart2.Instance = USART2;
  huart2.Init.BaudRate = 115200;
  huart2.Init.WordLength = UART_WORDLENGTH_8B;
  huart2.Init.StopBits = UART_STOPBITS_1;
  huart2.Init.Parity = UART_PARITY_NONE;
  huart2.Init.Mode = UART_MODE_TX_RX;
  huart2.Init.HwFlowCtl = UART_HWCONTROL_NONE;
  huart2.Init.OverSampling = UART_OVERSAMPLING_16;
  if (HAL_UART_Init(&huart2) != HAL_OK)
  {
    Error_Handler();
  }
  /* USER CODE BEGIN USART2_Init 2 */

  /* USER CODE END USART2_Init 2 */

}

/**
  * Enable DMA controller clock
  */
static void MX_DMA_Init(void)
{

  /* DMA controller clock enable */
  __HAL_RCC_DMA1_CLK_ENABLE();
  __HAL_RCC_DMA2_CLK_ENABLE();

  /* DMA interrupt init */
  /* DMA1_Stream5_IRQn interrupt configuration */
  HAL_NVIC_SetPriority(DMA1_Stream5_IRQn, 0, 0);
  HAL_NVIC_EnableIRQ(DMA1_Stream5_IRQn);
  /* DMA1_Stream6_IRQn interrupt configuration */
  HAL_NVIC_SetPriority(DMA1_Stream6_IRQn, 0, 0);
  HAL_NVIC_EnableIRQ(DMA1_Stream6_IRQn);
  /* DMA2_Stream0_IRQn interrupt configuration */
  HAL_NVIC_SetPriority(DMA2_Stream0_IRQn, 0, 0);
  HAL_NVIC_EnableIRQ(DMA2_Stream0_IRQn);

}

/**
  * @brief GPIO Initialization Function
  * @param None
  * @retval None
  */
static void MX_GPIO_Init(void)
{
  /* USER CODE BEGIN MX_GPIO_Init_1 */

  /* USER CODE END MX_GPIO_Init_1 */

  /* GPIO Ports Clock Enable */
  __HAL_RCC_GPIOC_CLK_ENABLE();
  __HAL_RCC_GPIOH_CLK_ENABLE();
  __HAL_RCC_GPIOA_CLK_ENABLE();
  __HAL_RCC_GPIOB_CLK_ENABLE();

  /* USER CODE BEGIN MX_GPIO_Init_2 */

  /* USER CODE END MX_GPIO_Init_2 */
}

/* USER CODE BEGIN 4 */

void HAL_UARTEx_RxEventCallback(UART_HandleTypeDef *huart, uint16_t size) {
	uint16_t new_position;
	uint16_t index;

	if (huart->Instance != USART2)
		return;

	uart_rx_events++;

	/*
	 *
	 * In circular DMA mode, `size` is the current position in the DMA
	 * buffer. At transmfer_complete it can equal UART_DMA_RX_CAPACITY.
	 */
	new_position = (uint16_t)(size % UART_DMA_RX_CAPACITY);

	if (new_position > uart_dma_last_position) {
		for (index = uart_dma_last_position; index < new_position; index++)
			uart_ring_push(uart_dma_rx[index]);
	}
	else if (new_position < uart_dma_last_position) {
		for (index = uart_dma_last_position; index < UART_DMA_RX_CAPACITY; index++)
			uart_ring_push(uart_dma_rx[index]);

		for (index = 0u; index < new_position; index++)
			uart_ring_push(uart_dma_rx[index]);
	}

	uart_dma_last_position = new_position;
}

static void uart_ring_push(uint8_t byte)
{
  uint16_t next_head = (uint16_t)((uart_ring_head + 1u) % UART_RING_CAPACITY);

  if (next_head == uart_ring_tail)
  {
    uart_ring_overflow = 1u;
    uart_rx_dropped_byte++;
    return;
  }

  uart_ring[uart_ring_head] = byte;
  uart_ring_head = next_head;
}

static bool uart_ring_pop(uint8_t *byte)
{
  if (uart_ring_tail == uart_ring_head)
  {
    return false;
  }

  *byte = uart_ring[uart_ring_tail];
  uart_ring_tail = (uint16_t)((uart_ring_tail + 1u) % UART_RING_CAPACITY);

  return true;
}

static void process_uart_byte(uint8_t byte)
{
  uint8_t last_was_cr = uart_last_was_cr;

  uart_last_was_cr = (byte == (uint8_t)'\r') ? 1u : 0u;

  if (byte == (uint8_t)'\n' && last_was_cr != 0u)
	  return;

  if (byte == (uint8_t)'\n' || byte == (uint8_t)'\r')
  {
    if (uart_discard_command != 0u)
    {
      uart_discard_command = 0u;
      uart_command_len = 0u;
      uart_send_text("ERROR: command too long\r\n> ");
      return;
    }

    uart_command[uart_command_len] = '\0';
    process_command(uart_command);
    uart_command_len = 0u;
    return;
  }

  if (uart_discard_command != 0u)
  {
    return;
  }

  if (uart_command_len < (UART_COMMAND_CAPACITY - 1u))
  {
    uart_command[uart_command_len] = (char)byte;
    uart_command_len++;
  }
  else
  {
    uart_discard_command = 1u;
    uart_rx_command_overflows++;
  }
}

static void process_command(const char *command)
{
	char *end_ptr;
	unsigned long requested_period;

	if(strcmp(command, "HELP") == 0)
	{
		uart_send_text("Commands:\r\n");
		uart_send_text(" LED ON\r\n");
		uart_send_text(" LED OFF\r\n");
		uart_send_text(" BLINK <100-2000>\r\n");
		uart_send_text(" BLINK OFF\r\n");
		uart_send_text(" STATUS\r\n");
		uart_send_text(" STATS\r\n");
		uart_send_text(" RESET STATS\r\n");
		uart_send_text(" CONFIG SHOW\r\n");
		uart_send_text(" CONFIG SAVE\r\n");
		uart_send_text(" CONFIG RESET\r\n");
		uart_send_text(" ADC\r\n> ");
	}
	else if (strcmp(command, "LED ON") == 0)
	{
		blink_enable = false;
		set_led(true);
		uart_send_text("OK: LED ON\r\n> ");
	}
	else if (strcmp(command, "LED OFF") == 0)
	{
		blink_enable = false;
		set_led(false);
		uart_send_text("OK: LED OFF\r\n> ");
	}
	else if (strcmp(command, "STATUS") == 0)
	{
		send_status();
	}
	else if (strcmp(command, "STATS") == 0)
	{
		send_stats();
	}
	else if (strcmp(command, "RESET STATS") == 0)
	{
		reset_stats();
	}
	else if (strcmp(command, "CONFIG SHOW") == 0)
	{
		send_config();
	}
	else if (strcmp(command, "CONFIG SAVE") == 0)
	{
		if (config_save())
			uart_send_text("OK: config saved\r\n> ");
		else
			uart_send_text("ERROR: flash save failed\r\n> ");
	}
	else if (strcmp(command, "CONFIG RESET") == 0)
	{
		if (config_erase()){
			blink_period_ms = BLINK_DEFAULT_MS;
			config_loaded_from_flash = false;
			uart_send_text("OK: config reset to defaults\r\n> ");
		}
		else
			uart_send_text("ERROR: flash erase failed\r\n> ");
	}
	else if (strcmp(command, "ADC") == 0)
	{
			send_adc();
	}
	else if (strcmp(command, "BLINK OFF") == 0)
	{
		blink_enable = false;
		uart_send_text("OK: blink disabled\r\n> ");
	}
	else if (strncmp(command, "BLINK ", 6u) == 0)
	{
		requested_period = strtoul(&command[6], &end_ptr, 10);

		if ((*end_ptr != '\0') || (requested_period < BLINK_MIN_PERIOD_MS) || (requested_period > BLINK_MAX_PERIOD_MS))
		{
			uart_send_text("ERROR: use BLINK <100-2000> or BLINK OFF\r\n> ");
			return;
		}

		blink_period_ms = requested_period;
		last_blink_ms = HAL_GetTick();
		blink_enable = true;

		uart_send_text("OK: blink enable\r\n> ");
	}
	else if (command[0] == '\0')
	{
		uart_send_text("> ");
	}
	else
	{
		uart_send_text("ERROR: unknown command; type HELP\r\n> ");
	}
}

static void set_led(bool on)
{
  if (on)
  {
    BSP_LED_On(LED2);
  }
  else
  {
    BSP_LED_Off(LED2);
  }

  led_is_on = on;
}

static void uart_send_text(const char *text)
{
  uint32_t primask;
  size_t len;
  uint16_t index;

  len = strlen(text);

  /*
   *
   * Every response in this project is much shorter than 255 bytes.
   * Refuse messages that can never fit in the ring.
   */
  if (len == 0u || len >= UART_TX_RING_CAPACITY) return;

  /*
   *
   * The main loop modified head while the UART-complete ISR modified tail.
   * Protect the check, enqueue, and DMA-start operation as one transaction.
   */
  primask = __get_PRIMASK();
  __disable_irq();

  if ((uint16_t)len > uart_tx_free_space_locked())
	  uart_tx_dropped_messages++;
  else {
	  for (index = 0u; index < (uint16_t)len; index++) {
		  uart_tx_ring[uart_tx_head] = (uint8_t)text[index];
		  uart_tx_head = (uint16_t)(uart_tx_head + 1u) % UART_TX_RING_CAPACITY;
	  }

	  uart_tx_kick_locked();
  }

  if (primask == 0u) __enable_irq();
}

static uint16_t uart_tx_free_space_locked(void) {
	uint16_t used;

	if (uart_tx_head >= uart_tx_tail)
		used = (uint16_t)(uart_tx_head - uart_tx_tail);
	else
		used = (uint16_t)(UART_TX_RING_CAPACITY - (uart_tx_tail - uart_tx_head));

	return (uint16_t)((UART_TX_RING_CAPACITY - 1u) - used);
}

static void uart_tx_kick_locked(void) {
	uint16_t contiguous_len;

	if ((uart_tx_dma_active != 0u) || (uart_tx_head == uart_tx_tail)) return;

	/*
	 *
	 * DMA can only read one contiguous memory range at a time.
	 * if the queued message wraps around the end of ring, send
	 * the tail-to-end segment now; TxCpltCallback starts the next one.
	 */
	if (uart_tx_head > uart_tx_tail)
		contiguous_len = (uint16_t)(uart_tx_head - uart_tx_tail);
	else
		contiguous_len = (uint16_t)(UART_TX_RING_CAPACITY - uart_tx_tail);

	uart_tx_dma_len = contiguous_len;
	uart_tx_dma_active = 1u;

	if (HAL_UART_Transmit_DMA(&huart2, &uart_tx_ring[uart_tx_tail], contiguous_len) != HAL_OK) {
		/*
		 *
		 * Keep the queued data intact. A later call to uart_send_text()
		 * can retry starting TX after transient HAL_BUSY conditions.
		 */
		uart_tx_dma_active = 0u;
		uart_tx_dma_len = 0u;
	}
}

void HAL_UART_TxCpltCallback(UART_HandleTypeDef *huart) {
	if (huart->Instance != USART2) return;

	/*
	 * The DMA transfer fully complete, so its ring-buffer region
	 * may now be reused by future responses.
	 */
	uart_tx_tail = (uint16_t)(uart_tx_tail + uart_tx_dma_len) % UART_TX_RING_CAPACITY;

	uart_tx_dma_len = 0u;
	uart_tx_dma_active = 0u;

	/*
	 *
	 * Start the next contiguous queued segment, if any.
	 * This callback already runs in interrupt context.
	 */
	uart_tx_kick_locked();
}

static void run_blink_task(void) {
	uint32_t now_ms = HAL_GetTick();

	/*
	 *
	 * Unsigned subtraction remains safe even when the 32-bit tick counter eventually wraps around.
	 */
	if (blink_enable && ((uint32_t)(now_ms - last_blink_ms)) >= blink_period_ms) {
		last_blink_ms = now_ms;
		set_led(!led_is_on);
	}
}

static void send_status(void) {
	char response[96];

	if (blink_enable) {
		(void)snprintf(response,
					   sizeof(response),
					   "STATUS: LED %s, BLINK ON (%lu ms)\r\n> ",
					   led_is_on ? "ON" : "OFF", (unsigned long)blink_period_ms);
	}
	else {
		(void)snprintf(response, sizeof(response),
						"STATUS: LED %s, BLINK OFF\r\n> ",
						led_is_on ? "ON" : "OFF");
	}

	uart_send_text(response);
}

static void send_stats(void) {
	char response[192];
	uint32_t uptime_ms = HAL_GetTick();

	(void)snprintf(response,
				   sizeof(response),
				   "STATS: uptime=%lu ms, RX events=%lu, RX dropped bytes=%lu, RX command overflows=%lu, TX dropped messages=%lu\r\n> ",
				   (unsigned long)uptime_ms,
				   (unsigned long)uart_rx_events,
				   (unsigned long)uart_rx_dropped_byte,
				   (unsigned long)uart_rx_command_overflows,
				   (unsigned long)uart_tx_dropped_messages);

	uart_send_text(response);
}

static void reset_stats(void) {
	uint32_t primask;

	primask = __get_PRIMASK();
	__disable_irq();

	uart_rx_events = 0u;
	uart_rx_dropped_byte = 0u;
	uart_rx_command_overflows = 0u;
	uart_tx_dropped_messages = 0u;

	if (primask == 0u) __enable_irq();

	uart_send_text("OK: stats reset\r\n> ");

}

static uint32_t config_calculate_checksum(const app_config_t *config) {
	return config->magic ^ config->blink_period_ms ^ config->version ^ 930203u;
}

static bool config_is_valid(const app_config_t *config) {
	if (config->magic != CONFIG_MAGIC)
		return false;
	else if (config->version != CONFIG_VERSION)
		return false;
	else if (config->checksum != config_calculate_checksum(config))
		return false;
	else if (config->blink_period_ms > BLINK_MAX_PERIOD_MS || config->blink_period_ms < BLINK_MIN_PERIOD_MS)
		return false;
	else
		return true;
}

static void config_load(void) {
	const app_config_t *config = (const app_config_t *)CONFIG_FLASH_ADDRESS;

	if (config_is_valid(config)) {
		blink_period_ms = config->blink_period_ms;
		config_loaded_from_flash = true;
	}

}

static void send_config(void) {
	char response[128];

	const app_config_t *config = (const app_config_t *)CONFIG_FLASH_ADDRESS;

	if (config_is_valid(config)) {
		(void)snprintf(response,
					   sizeof(response),
					   "Blink Period(Now)=%lu, Blink Period(In Flash)=%lu, Config Loaded From %s\r\n> ",
					   (unsigned long)blink_period_ms,
					   (unsigned long)config->blink_period_ms,
					   config_loaded_from_flash ? "Flash" : "Defaults");
	}
	else {
		(void)snprintf(response,
					   sizeof(response),
					   "Blink Period(Now)=%lu, Blink Period(In Flash)=%s, Config Loaded From %s\r\n> ",
					   (unsigned long)blink_period_ms,
					   "None",
					   config_loaded_from_flash ? "Flash" : "Defaults");
	}

	uart_send_text(response);
}

static bool config_save(void) {
	app_config_t new_config;
	bool success = false;

	FLASH_EraseInitTypeDef EraseInitStruct = {0};
	uint32_t sector_error = 0u;
	HAL_StatusTypeDef status;

	uint32_t index;
	const uint32_t *words;

	new_config.magic = CONFIG_MAGIC;
	new_config.version = CONFIG_VERSION;
	new_config.blink_period_ms = blink_period_ms;
	new_config.checksum = config_calculate_checksum(&new_config);

	words = (const uint32_t *)&new_config;

	status = HAL_FLASH_Unlock();

	if (status == HAL_OK) {
		EraseInitStruct.TypeErase = FLASH_TYPEERASE_SECTORS;
		EraseInitStruct.Sector = CONFIG_FLASH_SECTOR;
		EraseInitStruct.NbSectors = 1u;
		EraseInitStruct.VoltageRange = FLASH_VOLTAGE_RANGE_3;

		status = HAL_FLASHEx_Erase(&EraseInitStruct, &sector_error);

		if (status == HAL_OK) {
			for (index = 0u; index < (sizeof(app_config_t) / sizeof(uint32_t)); index++) {
				status = HAL_FLASH_Program(FLASH_TYPEPROGRAM_WORD, CONFIG_FLASH_ADDRESS + index*4u, words[index]);
				if (status != HAL_OK) break;
			}
		}
	}

	HAL_FLASH_Lock();

	if (status == HAL_OK) {
		const app_config_t *stored = (const app_config_t *)CONFIG_FLASH_ADDRESS;
		if (config_is_valid(stored) && memcmp(stored, &new_config, sizeof(app_config_t)) == 0)
			success = true;
	}

	return success;
}

static bool config_erase(void) {
	bool success = false;

	FLASH_EraseInitTypeDef EraseInitStruct = {0};
	uint32_t sector_error = 0u;
	HAL_StatusTypeDef status;

	status = HAL_FLASH_Unlock();

	if (status == HAL_OK) {
		EraseInitStruct.TypeErase = FLASH_TYPEERASE_SECTORS;
		EraseInitStruct.Sector = CONFIG_FLASH_SECTOR;
		EraseInitStruct.NbSectors = 1u;
		EraseInitStruct.VoltageRange = FLASH_VOLTAGE_RANGE_3;

		status = HAL_FLASHEx_Erase(&EraseInitStruct, &sector_error);
	}

	HAL_FLASH_Lock();

	if (status == HAL_OK) {
		const app_config_t *stored = (const app_config_t *)CONFIG_FLASH_ADDRESS;
		if (!config_is_valid(stored))
			success = true;
	}

	return success;
}

void HAL_ADC_ConvCpltCallback(ADC_HandleTypeDef *hadc) {
	if (hadc->Instance == ADC1)
		adc_sample_count++;
}

static void send_adc(void) {
	char response[64];
	uint16_t raw = adc_dma_buffer[0];
	uint32_t samples = adc_sample_count;

	(void)snprintf(response,
				   sizeof(response),
				   "ADC: raw=%lu, samples=%lu\r\n> ",
				   (unsigned long)raw,
				   (unsigned long)samples);

	uart_send_text(response);
}

/* USER CODE END 4 */

/**
  * @brief  This function is executed in case of error occurrence.
  * @retval None
  */
void Error_Handler(void)
{
  /* USER CODE BEGIN Error_Handler_Debug */
  /* User can add his own implementation to report the HAL error return state */
  __disable_irq();
  while (1)
  {
  }
  /* USER CODE END Error_Handler_Debug */
}
#ifdef USE_FULL_ASSERT
/**
  * @brief  Reports the name of the source file and the source line number
  *         where the assert_param error has occurred.
  * @param  file: pointer to the source file name
  * @param  line: assert_param error line source number
  * @retval None
  */
void assert_failed(uint8_t *file, uint32_t line)
{
  /* USER CODE BEGIN 6 */
  /* User can add his own implementation to report the file name and line number,
     ex: printf("Wrong parameters value: file %s on line %d\r\n", file, line) */
  /* USER CODE END 6 */
}
#endif /* USE_FULL_ASSERT */
