/*
 * adc.h
 *
 * Created on: Oct 14, 2026
 * Author: lepo9
 */

#ifndef ADC_H_
#define ADC_H_

#include <stdint.h>

void pa1_adc_init(void);
uint32_t adc_read(void);
void start_convertion(void);
void pa1_adc_interrupt_init(void);

/* Bandera EOC del registro ADC_ISR: bit 2. */
#define SR_EOC (1U << 2)

#endif /* ADC_H_ */
