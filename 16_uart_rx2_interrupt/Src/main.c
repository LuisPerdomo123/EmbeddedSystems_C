#include <stdio.h>
#include <stdint.h>
#include "stm32l4xx.h"
#include "uart.h"

#define GPIOAEN		(1U<<0)
#define GPIOA_5		(1U<<5)

#define LED_PIN		GPIOA_5

char key;

static void uart_callback(void);

int main(void){

	/*Enable clock access to GPIOA and GPIOC*/
	RCC->AHB2ENR |= GPIOAEN;

	/*Set PA5 as output pin*/
	GPIOA->MODER |= (1U<<10);
	GPIOA->MODER &=~ (1U<<11);

	uart2_rx_interrupt_init();

    while(1){



    }
}

static void uart_callback(void){
	key = USART2->RDR;

	if(key == '1'){
		GPIOA->ODR |= LED_PIN;
	}else{
		GPIOA->ODR &=~ LED_PIN;
	}
}

void USART2_IRQHandler(void){
	/*Check if RXNE is set*/
	if(USART2->ISR & ISR_RXNE){
		//Do something
		uart_callback();

	}
}
