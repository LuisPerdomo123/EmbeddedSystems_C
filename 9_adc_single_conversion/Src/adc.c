
#include <stdint.h>
#include "stm32l4xx.h"
#include "adc.h"

#define GPIOAEN          (1U<<0)
#define ADC1EN           (1U<<13)

#define ADC_CH6          (6U<<6)
#define ADC_SEQ_LEN_1    0x00U
#define CR_ADON          (1U<<0)
#define CR2_ADSTART      (1U<<2)
#define SR_EOC           (1U<<2)


void pa1_adc_init(void){

    /*** Configure the ADC GPIO pin ***/

    /* Enable clock access to GPIOA */
    RCC->AHB2ENR |= GPIOAEN;

    /* Set the mode of PA1 to analog */
    GPIOA->MODER |= (1U<<2);
    GPIOA->MODER |= (1U<<3);

    /* Enable analog switch PA1 */
    GPIOA->ASCR |= (1U<<1);

    /*** Configure the ADC module ***/

    /* Enable clock access to ADC */
    RCC->AHB2ENR |= ADC1EN;

    /* NUEVO: Select ADC clock HCLK/2 */
    ADC123_COMMON->CCR &= ~ADC_CCR_CKMODE;
    ADC123_COMMON->CCR |= ADC_CCR_CKMODE_1;

    /* NUEVO: Exit Deep-power-down */
    ADC1->CR &= ~ADC_CR_DEEPPWD;

    /* NUEVO: Enable ADC voltage regulator */
    ADC1->CR |= ADC_CR_ADVREGEN;

    /* NUEVO: Wait for regulator stabilization */
    for(volatile uint32_t i=0; i<4000U; i++){
        __NOP();
    }

    /* NUEVO: Calibrate ADC */
    ADC1->CR |= ADC_CR_ADCAL;

    while(ADC1->CR & ADC_CR_ADCAL){}

    /* NUEVO: Wait after calibration */
    for(volatile uint32_t i=0; i<100U; i++){
        __NOP();
    }

    /* NUEVO: Clear ADC ready flag */
    ADC1->ISR = ADC_ISR_ADRDY;

    /* Enable ADC module */
    ADC1->CR |= CR_ADON;

    /* NUEVO: Wait until ADC is ready */
    while(!(ADC1->ISR & ADC_ISR_ADRDY)){}

    /* NUEVO: Sampling time for channel 6 */
    ADC1->SMPR1 |= ADC_SMPR1_SMP6_2;

    /* Conversion sequence length */
    ADC1->SQR1 = ADC_SEQ_LEN_1;

    /* Conversion sequence start */
    ADC1->SQR1 |= ADC_CH6;
}


void start_convertion(void){

    /* NUEVO: Clear previous EOC flag */
    ADC1->ISR = SR_EOC;

    /* Start ADC conversion */
    ADC1->CR |= CR2_ADSTART;
}


uint32_t adc_read(void){

    /* Wait for conversion to be complete */
    while(!(ADC1->ISR & SR_EOC)){}

    /* Read converted result */
    return (ADC1->DR);
}
