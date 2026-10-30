/*
 * timer.h
 *
 *  Created on: Oct 22, 2026
 *      Author: lepo9
 */

#ifndef TIMER_H_
#define TIMER_H_

void tim2_1hz_init(void);

#define SR_UIF		(1U<<0)
void tim2_1hz_interrupt_init(void);


#endif /* TIMER_H_ */
