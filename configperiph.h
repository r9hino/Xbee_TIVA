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

void ConfigureTimer0(uint16_t timePeriod);
void ConfigureUART0(void);
void ConfigureUART1(void);
void ConfigureI2C3(void);

//*****************************************************************************
// Mark the end of the C bindings section for C++ compilers.
//*****************************************************************************
#ifdef __cplusplus
}
#endif

#endif /* INIT_CONFIG_H_ */
