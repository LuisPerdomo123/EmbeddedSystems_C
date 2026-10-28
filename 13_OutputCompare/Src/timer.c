#include "stm32l4xx.h"


#define TIM2EN			(1U<<0)
#define CR1_CEN			(1U<<0)
#define OC_TOGGLE		((1U<<4) | (1U<<5))
#define CCER_CC1E		(1U<<0)

#define GPIOAEN			(1U<<0)
#define AFR5_TIM		(1U<<20)


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

void tim2_pa5_output_compare(void){

	/*Enable clock access to GPIOA*/
	RCC->AHB2ENR |= GPIOAEN;
	/*Set PA5 mode alternate function*/
	GPIOA->MODER &= ~(1U<<10); // CORRECCION 1: bit 10 = 0 (MODER5[0])
	GPIOA->MODER |=  (1U<<11); // CORRECCION 2: bit 11 = 1 (MODER5[1]); 10 = AF
	/*Set PA5 alternate function type to TIM2_CH1 (AF1)*/
	/* MODIFICACION 2: limpiar solo AFRL5
	       para no modificar las AF de otros pines. */
	GPIOA->AFR[0] &= ~(0xFU<<20);
	GPIOA->AFR[0] |= AFR5_TIM;

	/*Enable clock access to tim2*/
	RCC->APB1ENR1 |= TIM2EN;
	/*Set prescaler value*/
	TIM2->PSC = 400-1;		//4 000 000/ 400 = 10 000
	/*Set auto-reload value*/
	TIM2->ARR = 10000-1;	//10 000/10 000 = 1

	/*Set output compare toggle mode*/
	TIM2->CCMR1 = OC_TOGGLE;
	/*Enable tim2 ch1 compare mode*/

	/* MODIFICACION 3: definir explicitamente
	       CCR1 dentro de 0..ARR.
	       La coincidencia CNT=5000 ocurre a los
	       0,5 s inicialmente y luego cada 1 s. */
	TIM2->CCR1 = 5000;

	TIM2->CCER |= CCER_CC1E;



	/*Clear counter*/
	TIM2->CNT = 0;

	TIM2->CR1 |= TIM_CR1_URS; // NUEVA
	TIM2->EGR = TIM_EGR_UG;   // NUEVA
	TIM2->SR &= ~TIM_SR_UIF;  // NUEVA

	/*Enable the timer*/
	TIM2->CR1 |= CR1_CEN;
}
