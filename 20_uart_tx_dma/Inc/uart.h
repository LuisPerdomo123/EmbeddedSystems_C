#ifndef UART_H_
#define UART_H_

#include <stdint.h>
#include "stm32l4xx.h"

void uart2_tx_init(void);
void uart2_rxtx_init(void);
void uart2_rx_interrupt_init(void);

char uart2_read(void);
void uart2_write(int ch);
int __io_putchar(int ch);

void dma1_CxS2_init(
    uint32_t src,
    uint32_t dst,
    uint32_t len
);

/* Se conservan los nombres utilizados anteriormente. */
#define ISR_RXNE       USART_ISR_RXNE
#define ISR_TCIF7      DMA_ISR_TCIF7
#define IFCR_CTCIF7    DMA_IFCR_CTCIF7

#endif /* UART_H_ */
