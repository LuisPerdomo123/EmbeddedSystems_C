#include "exti.h"


#define GPIOCEN		(1U<<2)
#define SYSCFGEN	(1U<<0)

void pc13_exti_init(void){
	/*Disable global interrupt*/
	__disable_irq();
	/*Enable clock access for GPIOC*/
	RCC->AHB2ENR |= GPIOCEN;
	/*Set PC13 as input*/
	GPIOC->MODER &=~ (1U<<26);
	GPIOC->MODER &=~ (1U<<27);
	/*Enable clock access to SYSCFG*/
	RCC->APB2ENR |= SYSCFGEN;
	(void)RCC->APB2ENR; /* Synchronize peripheral clock */
	/*Select PORTC for EXTI13*/
	SYSCFG->EXTICR[3] &= ~(0xFU<<4);
	SYSCFG->EXTICR[3] |=  (1U<<5);
	/*Unmask EXTI13*/
	EXTI->IMR1 |= (1U<<13);
	/*Select falling edge trigger*/
	EXTI->RTSR1 &= ~(1U<<13);
	EXTI->FTSR1 |= (1U<<13);
	/*Clear possible pending event before NVIC activation*/
	EXTI->PR1 = (1U<<13);
	/*Enable EXTI13 line in NVIC*/
	NVIC_EnableIRQ(EXTI15_10_IRQn);
	/*Enable global interrupts*/
	__enable_irq();

}
