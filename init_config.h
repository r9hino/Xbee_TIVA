/*
 * init_config.h
 *
 *  Created on: 28-10-2014
 *      Author: r9hino
 */

#ifndef INIT_CONFIG_H_
#define INIT_CONFIG_H_

// Timers
#define TIMER_PERIOD_5SEC					5
#define TIMER_PERIOD_10SEC				   10
#define TIMER_PERIOD_15SEC				   15
#define TIMER_PERIOD_30SEC				   30
#define TIMER_PERIOD_60SEC				   60

void ConfigureTimer0(void);
void ConfigureUART0(void);
void ConfigureUART1(void);

#endif /* INIT_CONFIG_H_ */
