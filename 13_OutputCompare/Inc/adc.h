/*
 * adc.h
 *
 *  Created on: Oct 14, 2026
 *      Author: lepo9
 */

#ifndef ADC_H_
#define ADC_H_
#include <stdint.h>

void pa1_adc_init(void);
uint32_t adc_read(void);
void start_convertion(void);


#endif /* ADC_H_ */

