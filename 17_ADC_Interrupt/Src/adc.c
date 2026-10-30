#include <stdint.h>
#include "stm32l4xx.h"
#include "adc.h"

#define GPIOAEN          (1U << 0)
#define ADC1EN           (1U << 13)

#define ADC_CH6          (6U << 6)
#define ADC_SEQ_LEN_1    0x00U

#define CR_ADON          (1U << 0)
#define CR2_ADSTART      (1U << 2)

#define CFGR_CONT        (1U << 13)

/* Fin de conversion regular: EOCIE corresponde al bit 2. */
#define IER_EOCIE        ADC_IER_EOCIE

void pa1_adc_interrupt_init(void)
{
    /* Habilitar reloj de GPIOA. */
    RCC->AHB2ENR |= GPIOAEN;

    /* Configurar PA1 en modo analogico. */
    GPIOA->MODER |= (1U << 2);
    GPIOA->MODER |= (1U << 3);

    /* Habilitar el interruptor analogico de PA1. */
    GPIOA->ASCR |= (1U << 1);

    /* Habilitar reloj del ADC. */
    RCC->AHB2ENR |= ADC1EN;

    /* Habilitar interrupcion por fin de conversion regular. */
    ADC1->IER |= IER_EOCIE;

    /* Habilitar la interrupcion compartida ADC1/ADC2 en el NVIC. */
    NVIC_EnableIRQ(ADC1_2_IRQn);

    /* Seleccionar reloj del ADC: HCLK/2. */
    ADC123_COMMON->CCR &= ~ADC_CCR_CKMODE;
    ADC123_COMMON->CCR |= ADC_CCR_CKMODE_1;

    /* Salir del modo Deep-power-down. */
    ADC1->CR &= ~ADC_CR_DEEPPWD;

    /* Habilitar el regulador de tension del ADC. */
    ADC1->CR |= ADC_CR_ADVREGEN;

    /* Espera original para estabilizacion del regulador. */
    for (volatile uint32_t i = 0; i < 4000U; i++)
    {
        __NOP();
    }

    /* Iniciar calibracion del ADC. */
    ADC1->CR |= ADC_CR_ADCAL;

    /* Esperar a que finalice la calibracion. */
    while (ADC1->CR & ADC_CR_ADCAL)
    {
    }

    /* Espera original posterior a la calibracion. */
    for (volatile uint32_t i = 0; i < 100U; i++)
    {
        __NOP();
    }

    /* Limpiar la bandera ADC ready escribiendo un 1. */
    ADC1->ISR = ADC_ISR_ADRDY;

    /* Habilitar ADC1. */
    ADC1->CR |= CR_ADON;

    /* Esperar a que ADC1 este listo. */
    while (!(ADC1->ISR & ADC_ISR_ADRDY))
    {
    }

    /* Mantener el tiempo de muestreo original del canal 6. */
    ADC1->SMPR1 |= ADC_SMPR1_SMP6_2;

    /* Secuencia de una conversion. */
    ADC1->SQR1 = ADC_SEQ_LEN_1;

    /* Canal 6 como primera conversion de la secuencia. */
    ADC1->SQR1 |= ADC_CH6;
}

void pa1_adc_init(void)
{
    /* Inicializacion original para uso por sondeo. */

    /* Habilitar reloj de GPIOA. */
    RCC->AHB2ENR |= GPIOAEN;

    /* Configurar PA1 en modo analogico. */
    GPIOA->MODER |= (1U << 2);
    GPIOA->MODER |= (1U << 3);

    /* Habilitar el interruptor analogico de PA1. */
    GPIOA->ASCR |= (1U << 1);

    /* Habilitar reloj del ADC. */
    RCC->AHB2ENR |= ADC1EN;

    /* Seleccionar reloj del ADC: HCLK/2. */
    ADC123_COMMON->CCR &= ~ADC_CCR_CKMODE;
    ADC123_COMMON->CCR |= ADC_CCR_CKMODE_1;

    /* Salir del modo Deep-power-down. */
    ADC1->CR &= ~ADC_CR_DEEPPWD;

    /* Habilitar el regulador de tension del ADC. */
    ADC1->CR |= ADC_CR_ADVREGEN;

    /* Espera original para estabilizacion del regulador. */
    for (volatile uint32_t i = 0; i < 4000U; i++)
    {
        __NOP();
    }

    /* Iniciar calibracion del ADC. */
    ADC1->CR |= ADC_CR_ADCAL;

    /* Esperar a que finalice la calibracion. */
    while (ADC1->CR & ADC_CR_ADCAL)
    {
    }

    /* Espera original posterior a la calibracion. */
    for (volatile uint32_t i = 0; i < 100U; i++)
    {
        __NOP();
    }

    /* Limpiar la bandera ADC ready escribiendo un 1. */
    ADC1->ISR = ADC_ISR_ADRDY;

    /* Habilitar ADC1. */
    ADC1->CR |= CR_ADON;

    /* Esperar a que ADC1 este listo. */
    while (!(ADC1->ISR & ADC_ISR_ADRDY))
    {
    }

    /* Mantener el tiempo de muestreo original del canal 6. */
    ADC1->SMPR1 |= ADC_SMPR1_SMP6_2;

    /* Secuencia de una conversion. */
    ADC1->SQR1 = ADC_SEQ_LEN_1;

    /* Canal 6 como primera conversion de la secuencia. */
    ADC1->SQR1 |= ADC_CH6;
}

void start_convertion(void)
{
    /*
     * Mantener conversion continua y sobrescritura del resultado
     * anterior cuando se produzca overrun.
     */
    ADC1->CFGR |= CFGR_CONT | ADC_CFGR_OVRMOD;

    /* Limpiar una posible bandera EOC anterior. */
    ADC1->ISR = SR_EOC;

    /* Iniciar las conversiones. */
    ADC1->CR |= CR2_ADSTART;
}

uint32_t adc_read(void)
{
    /* Funcion original para lectura por sondeo. */
    while (!(ADC1->ISR & SR_EOC))
    {
    }

    /* Leer el resultado; esta lectura tambien limpia EOC. */
    return ADC1->DR;
}
