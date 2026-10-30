#include "stm32l4xx.h"


#define TIM2EN			(1U<<0)
#define CR1_CEN			(1U<<0)
#define DIER_UIE		(1U<<0)


void tim2_1hz_init(void){
	/*Enable clock access to tim2*/
	RCC->APB1ENR1 |= TIM2EN;
	/*Set prescaler value*/
	TIM2->PSC = 400-1;		//4 000 000/ 400 = 10 000
	/*Set auto-reload value*/
	TIM2->ARR = 10000-1;	//10 000/10 000 = 1
	/*Clear counter*/
	TIM2->CNT = 0;

	TIM2->CR1 |= TIM_CR1_URS; // NUEVA
	TIM2->EGR = TIM_EGR_UG;   // NUEVA
	TIM2->SR &= ~TIM_SR_UIF;  // NUEVA

	/*Enable the timer*/
	TIM2->CR1 = CR1_CEN;
}


void tim2_1hz_interrupt_init(void){
	/*Enable clock access to tim2*/
	RCC->APB1ENR1 |= TIM2EN;
	/*Set prescaler value*/
	TIM2->PSC = 400-1;		//4 000 000/ 400 = 10 000
	/*Set auto-reload value*/
	TIM2->ARR = 10000-1;	//10 000/10 000 = 1
	/*Clear counter*/
	TIM2->CNT = 0;

	TIM2->CR1 |= TIM_CR1_URS; // NUEVA
	TIM2->EGR = TIM_EGR_UG;   // NUEVA
	TIM2->SR &= ~TIM_SR_UIF;  // NUEVA

	/*Enable the timer*/
	TIM2->CR1 = CR1_CEN;

	/*Enable TIM Interrupt*/
	TIM2->DIER |= DIER_UIE;
	/*Enable TIM Interrupt in NVIC*/
	NVIC_EnableIRQ(TIM2_IRQn);

}
