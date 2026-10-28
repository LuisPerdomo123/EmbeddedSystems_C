/*
 * exti.h
 *
 *  Created on: Oct 26, 2026
 *      Author: lepo9
 */

#ifndef EXTI_H_
#define EXTI_H_

#include "stm32l4xx.h"
void pc13_exti_init(void);

#define		LINE13	(1U<<13)

#endif /* EXTI_H_ */
