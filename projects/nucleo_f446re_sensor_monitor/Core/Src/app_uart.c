#include <app_uart.h>
#include <string.h>
#include <stdbool.h>
#include "app_watchdog.h"

#define TX_RING_CAPACITY 512u
#define DMA_RX_CAPACITY 64u

static UART_HandleTypeDef *huart2;
static uint8_t tx_ring[TX_RING_CAPACITY];
static volatile uint16_t tx_head = 0u;
static volatile uint16_t tx_tail = 0u;
static volatile uint16_t tx_dma_len = 0u;
static volatile uint8_t tx_dma_active = 0u;
static volatile uint32_t tx_dropped_messages = 0u;

static osMessageQueueId_t rxQueue = NULL;
static uint8_t dma_rx[DMA_RX_CAPACITY];
static volatile uint16_t dma_last_position = 0u;
static volatile uint32_t rx_events = 0u;
static volatile uint32_t rx_dropped_bytes = 0u;
static volatile uint8_t ring_overflow = 0u;

static uint16_t tx_free_space_locked(void);
static void tx_kick_locked(void);
static void rx_enqueue(uint8_t byte);

/* Public Part */
void app_uart_init(UART_HandleTypeDef *huart) {
	huart2 = huart;
}

void app_uart_send_text(const char *text) {
	uint32_t primask;
	size_t len;
	uint16_t index;

	len = strlen(text);

	// length must be longer than 0 bytes, or shorter than ring capacity.
	if ((len == 0u) || (len >= TX_RING_CAPACITY)) {
		return;
	}

	primask = __get_PRIMASK();
	__disable_irq();

	if ((uint16_t)len > tx_free_space_locked()) {
		tx_dropped_messages++;
	}
	else {
		for (index = 0; index < (uint16_t)len; index++) {
			tx_ring[tx_head] = text[index];
			tx_head = (uint16_t)(tx_head + 1u) % TX_RING_CAPACITY;
		}

		tx_kick_locked();
	}

	if (primask == 0u) {
		__enable_irq();
	}
}

void app_uart_start_rx(osMessageQueueId_t queue) {
	rxQueue = queue;

	if (HAL_UARTEx_ReceiveToIdle_DMA(huart2, dma_rx, DMA_RX_CAPACITY) != HAL_OK) {
		Error_Handler();
	}
}

bool app_uart_read_byte(uint8_t *byte, uint32_t timeout_ms) {
	if (osMessageQueueGet(rxQueue, byte, NULL, timeout_ms) != osOK) {
		return false;
	}
	else {
		return true;
	}
}

bool app_uart_take_rx_overflow(void) {
	uint32_t primask;
	bool overflow;

	primask = __get_PRIMASK();
	__disable_irq();

	overflow = (ring_overflow != 0u);
	ring_overflow = 0u;

	if (primask == 0u) {
		__enable_irq();
	}

	return overflow;
}

void app_uart_get_stats(app_uart_stats_t *out) {
	uint32_t primask;

	primask = __get_PRIMASK();
	__disable_irq();

	out->rx_events = rx_events;
	out->rx_dropped_bytes = rx_dropped_bytes;
	out->tx_dropped_messages = tx_dropped_messages;

	if (primask == 0u) {
		__enable_irq();
	}
}

void app_uart_reset_stats(void) {
	uint32_t primask;

	primask = __get_PRIMASK();
	__disable_irq();

	rx_events = 0u;
	rx_dropped_bytes = 0u;
	tx_dropped_messages = 0u;

	if (primask == 0u) {
		__enable_irq();
	}
}

/* Private Part */
static uint16_t tx_free_space_locked(void) {
	uint16_t used;

	if (tx_head >= tx_tail) {
		used = (uint16_t)(tx_head - tx_tail);
	}
	else {
		used = (uint16_t)(TX_RING_CAPACITY - (tx_tail - tx_head));
	}

	return (uint16_t)((TX_RING_CAPACITY - 1u) - used);
}

static void tx_kick_locked(void) {
	uint16_t contiguous_len;

	// DMA is active or Ring is empty
	if ((tx_dma_active != 0u) || (tx_head == tx_tail)) {
		return;
	}

	/*
	 * if the message wraps around the end of ring,
	 * send tail-to-end now, TxCpltCallback starts the next one.
	 */
	if (tx_head > tx_tail) {
		contiguous_len = (uint16_t)(tx_head - tx_tail);
	}
	else {
		contiguous_len = (uint16_t)(TX_RING_CAPACITY - tx_tail);
	}

	tx_dma_len = contiguous_len;
	tx_dma_active = 1u;

	if (HAL_UART_Transmit_DMA(huart2, &tx_ring[tx_tail], contiguous_len) != HAL_OK) {
		tx_dma_active = 0u;
		tx_dma_len = 0u;
	}
}

static void rx_enqueue(uint8_t byte) {
	if (osMessageQueuePut(rxQueue, &byte, 0, 0) != osOK) {
		ring_overflow = 1u;
		rx_dropped_bytes++;
	}
}

/* System Configuration */
void HAL_UART_TxCpltCallback(UART_HandleTypeDef *huart) {
	if (huart->Instance != USART2) {
		return;
	}

	// The DMA transfer fully complete.
	// Update ring-buffer region(tx_tail, tx_dma_active, tx_dam_len)
	tx_tail = (uint16_t)((tx_tail + tx_dma_len) % TX_RING_CAPACITY);
	tx_dma_active = 0u;
	tx_dma_len = 0u;

	//Start the next one, clean up the rest of message.
	tx_kick_locked();
}

void HAL_UARTEx_RxEventCallback(UART_HandleTypeDef *huart, uint16_t size) {
	uint16_t new_position;
	uint16_t index;

	if (huart != huart2) {
		return;
	}

	rx_events++;
	new_position = (uint16_t)(size % DMA_RX_CAPACITY);

	if (new_position > dma_last_position) {
		for (index = dma_last_position; index < new_position; index++) {
			rx_enqueue(dma_rx[index]);
		}
	}
	else if (new_position < dma_last_position) {
		for (index = dma_last_position; index < DMA_RX_CAPACITY; index++) {
			rx_enqueue(dma_rx[index]);
		}

		for (index = 0u; index < new_position; index++) {
			rx_enqueue(dma_rx[index]);
		}
	}

	dma_last_position = new_position;
}
