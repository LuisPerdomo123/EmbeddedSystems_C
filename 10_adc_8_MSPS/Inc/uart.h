/*
 * uart.h
 *
 *  Created on: Oct 12, 2026
 *      Author: lepo9
 */

#ifndef UART_H_
#define UART_H_

#include <stdint.h>
#include "stm32l4xx.h"

void uart2_tx_init(void);
char uart2_read(void);
void uart2_rxtx_init(void);


#endif /* UART_H_ */
