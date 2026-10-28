#include <stdio.h>
#include <stdint.h>
#include "stm32l4xx.h"
#include "uart.h"
#include "adc.h"
#include "systick.h"
#include "timer.h"
#include "exti.h"

#define GPIOAEN		(1U<<0)
#define PIN5		(1U<<5)

#define LED			PIN5

static void exti_callback(void);
static void uart2_send_string(const char *str);
/*Already implemented in the original uart.c*/
extern void uart2_write(int ch);

int main(void){

	/*1. Enable clock access to GPIOA*/
	RCC->AHB2ENR |= GPIOAEN;

	/*2. Set PA5 as output pin*/
	GPIOA->MODER |= (1U<<10);
	GPIOA->MODER &=~ (1U<<11);


	/*Visual confirmation of startup / black RESET button*/
	GPIOA->BSRR = LED;
	for(volatile uint32_t i=0; i<300000U; i++){ __NOP(); }
	GPIOA->BRR = LED;

	/*UART must be ready before EXTI can invoke its callback*/
	uart2_tx_init();
	uart2_send_string("MCU START / RESET\r\n");
	pc13_exti_init();

	while(1){

    }
}

static void uart2_send_string(const char *str){
	while(*str != '\0'){
		uart2_write((uint8_t)*str++);
	}
}

static void exti_callback(void){
	GPIOA->ODR ^= LED; /*Visual confirmation of EXTI13*/
	uart2_send_string("BTN Pressed...\r\n");
}



void EXTI15_10_IRQHandler(void){
	if((EXTI->PR1 & LINE13)!=0){
		/*Clear PR flag*/
		EXTI->PR1 = LINE13;
		//Do something...
		exti_callback();

	}
}
