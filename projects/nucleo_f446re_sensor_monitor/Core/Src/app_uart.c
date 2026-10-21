#include <app_uart.h>
#include <string.h>

#define TX_RING_CAPACITY 512u

static UART_HandleTypeDef *huart2;
static uint8_t tx_ring[TX_RING_CAPACITY];
static volatile uint16_t tx_head = 0u;
static volatile uint16_t tx_tail = 0u;
static volatile uint16_t tx_dma_len = 0u;
static volatile uint8_t tx_dma_active = 0u;
static volatile uint32_t tx_dropped_messages = 0u;

static uint16_t tx_free_space_locked(void);
static void tx_kick_locked(void);


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
