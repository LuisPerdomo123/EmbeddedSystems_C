/*
 * adc.h
 *
 *  Created on: Oct 14, 2026
 *      Author: lepo9
 */

#ifndef ADC_H_
#define ADC_H_
#include <stdint.h>

/* [C6] 8 MSPS * 50 us = 400 samples (200 paired DMA words). */
#define ADC_CAPTURE_SAMPLES 400U

void pa1_adc_init(void);
uint32_t adc_read(void);
void start_convertion(void);
int adc_capture_wait(void);
uint32_t adc_get_sample(uint32_t sample_index);
uint32_t adc_get_time_ns(uint32_t sample_index);

/* [F2] Snapshot of failure registers, read after capture (no UART during DMA). */
uint32_t adc_capture_error(void);
uint32_t adc_capture_dma_flags(void);
uint32_t adc_capture_adc1_flags(void);
uint32_t adc_capture_adc2_flags(void);
uint32_t adc_capture_remaining(void);


#endif /* ADC_H_ */

