#include "stm32l4xx.h"


#define TIM2EN			(1U<<0)
#define TIM3EN			(1U<<1)

#define CR1_CEN			(1U<<0)
#define OC_TOGGLE		((1U<<4) | (1U<<5))
#define CCER_CC1E		(1U<<0)

#define GPIOAEN			(1U<<0)
#define AFR5_TIM		(1U<<20)
#define AFR6_TIM		(1U<<25)
#define CCER_CC1S		(1U<<0)


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


void tim3_pa6_input_capture(void){

	/*Enable clock access to GPIOA*/
	RCC->AHB2ENR |= GPIOAEN;
	/*Set PA6 mode alternate function*/
	GPIOA->MODER &= ~(1U<<12);
	GPIOA->MODER |=  (1U<<13);
	/*Set PA6 alternate function type to TIM3_CH1 (AF2)*/
	/* CORRECCION 2: AFRL6 ocupa bits [27:24].
     * El desplazamiento previo (<<25) borraba [28:25],
     * dejaba intacto el bit 24 e invadia AFRL7 (PA7). */
	GPIOA->AFR[0] &= ~(0xFU<<24);
	GPIOA->AFR[0] |= AFR6_TIM;
	/*Enable clock access to tim3*/
	RCC->APB1ENR1 |= TIM3EN;
	/*Set prescaler*/
	TIM3->PSC = 400-1;
	/* CORRECCION 3: el prescaler tiene precarga (preload).
		 * UG transfiere PSC=399 al prescaler activo antes de empezar;
		 * asi el primer intervalo usa f_TIM3/(399+1).
		 * No se cambia ARR: TIM3 es de 16 bits y conserva su valor
		 * de reset (0xFFFF). Esto es deliberado para capturar marcas
		 * temporales, no forzar un periodo de 1 segundo. */
	TIM3->EGR = TIM_EGR_UG;
	TIM3->SR = 0;  /* Limpiar banderas generadas durante inicio. */
	TIM3->CNT = 0; /* Empezar la base de tiempos desde cero. */
	/*Set CH1 to input capture*/
	TIM3->CCMR1 = CCER_CC1S;
	/*Set CH1 to capture at rising edge*/
	TIM3->CCER = CCER_CC1E;
	/*Enable TIM3*/
	TIM3->CR1 |= CR1_CEN;

}
