/*
 * timer.h
 *
 *  Created on: Oct 22, 2026
 *      Author: lepo9
 */

#ifndef TIMER_H_
#define TIMER_H_

void tim2_1hz_init(void);
void tim2_pa5_output_compare(void);
void tim3_pa6_input_capture(void);


#define SR_UIF		(1U<<0)
#define SR_CC1IF	(1U<<1)


#endif /* TIMER_H_ */
