/*
 * init_config.h
 *
 *  Created on: 28-10-2014
 *      Author: r9hino
 */

#ifndef INIT_CONFIG_H_
#define INIT_CONFIG_H_

//*****************************************************************************
// If building with a C++ compiler, make all of the definitions in this header
// have a C binding.
//*****************************************************************************
#ifdef __cplusplus
extern "C"
{
#endif

// Timers
#define TIMER_PERIOD_5SEC					5
#define TIMER_PERIOD_10SEC				   10
#define TIMER_PERIOD_15SEC				   15
#define TIMER_PERIOD_30SEC				   30
#define TIMER_PERIOD_45SEC				   45
#define TIMER_PERIOD_300SEC				  300	// Too much 80 000 000*300 > 2^32

void ConfigureTimer0(void);
void ConfigureUART0(void);
void ConfigureUART1(void);

//*****************************************************************************
// Mark the end of the C bindings section for C++ compilers.
//*****************************************************************************
#ifdef __cplusplus
}
#endif

#endif /* INIT_CONFIG_H_ */
