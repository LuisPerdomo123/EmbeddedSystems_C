#include <stdio.h>
#include <stdint.h>
#include "stm32l4xx.h"
#include "uart.h"
#include "adc.h"

static void adc_callback(void);

/* Actualizada por la interrupcion y leida desde main. */
volatile uint32_t sensor_value;

int main(void)
{
    uart2_tx_init();
    pa1_adc_interrupt_init();
    start_convertion();

    while (1)
    {

    }
}

static void adc_callback(void)
{
    /* Leer DR obtiene el resultado y limpia la bandera EOC. */
    sensor_value = ADC1->DR;
    /* Transmitir fuera de la rutina de interrupcion. */
    printf("Sensor value : %d \n\r", (int)sensor_value);
}

void ADC1_2_IRQHandler(void)
{
    /* Comprobar fin de conversion regular de ADC1. */
    if ((ADC1->ISR & SR_EOC) != 0)
    {
        adc_callback();
    }
}
