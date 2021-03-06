#ifndef TIMERS_H
#define TIMERS_H

#include <libopencm3/stm32/timer.h>
#include "defines.h"
#define START_PWM_VALUE		90
//Break impulse in ms
#define BREAK_IMPULSE_LENTH	200

/*
 *Timers settings rules:
 *Tim1 - 39 kHz pwm freq
 *Tim2 - 50 hertz minimum measurement
 *Tim3 - 3 seconds
 */
#define TIMER1_TOP		1846
#define TIMER2_TOP		45000
#define TIMER3_TOP		3600	/*period = 3 sec*/
#define TIMER4_TOP		1200	/*period = 1 sec*/

//prescaler sets as "prescaler - 1"
#define TIMER1_PRESCALER	0
#define TIMER2_PRESCALER	31
#define TIMER3_PRESCALER	60000-1	
#define TIMER4_PRESCALER	60000-1	

void tim1_init(void);
void tim2_init(void);
void tim3_init(void);
void tim4_init(void);
void tim1_enable(uint8_t param);
void tim3_enable(uint8_t param);
void tim4_enable(uint8_t param);
void tim1_set_pwm (uint8_t pwm);
void tim3_set_pwm (uint16_t pwm);
void tim4_set_pwm (uint16_t pwm);
void systick_setup(void);

#endif
