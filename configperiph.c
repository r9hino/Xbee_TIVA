/*
 * init_config.c
 *
 *  Created on: 28-10-2014
 *      Author: r9hino
 */

#include <stdint.h>
#include <stdbool.h>
#include "inc/hw_ints.h"
#include "inc/hw_memmap.h"
#include "driverlib/gpio.h"
#include "driverlib/pin_map.h"
#include "driverlib/rom.h"
#include "driverlib/sysctl.h"
#include "driverlib/timer.h"
#include "driverlib/uart.h"
#include "utils/uartstdio.h"
#include "configperiph.h"

void ConfigureTimer0 (uint16_t timePeriod) {
	// Enable Timer0
	ROM_SysCtlPeripheralEnable(SYSCTL_PERIPH_TIMER0);
	ROM_TimerConfigure(TIMER0_BASE, TIMER_CFG_PERIODIC);
	ROM_TimerLoadSet(TIMER0_BASE, TIMER_A, timePeriod*ROM_SysCtlClockGet());
	ROM_TimerIntEnable(TIMER0_BASE, TIMER_TIMA_TIMEOUT);
	ROM_IntEnable(INT_TIMER0A);
	ROM_TimerEnable(TIMER0_BASE, TIMER_A);
}

void ConfigureUART0(void){
    // Enable the GPIO Peripheral used by the UART.
    ROM_SysCtlPeripheralEnable(SYSCTL_PERIPH_GPIOA);
    // Enable UART0
    ROM_SysCtlPeripheralEnable(SYSCTL_PERIPH_UART0);

	// Set GPIO A0 and A1 as UART pins.
	ROM_GPIOPinConfigure(GPIO_PA0_U0RX);
	ROM_GPIOPinConfigure(GPIO_PA1_U0TX);
	ROM_GPIOPinTypeUART(GPIO_PORTA_BASE, GPIO_PIN_0 | GPIO_PIN_1);

    // Configure UART clock using UART utils. The above line does not work by itself to enable the UART.
    ROM_UARTClockSourceSet(UART0_BASE, UART_CLOCK_PIOSC);
    ROM_UARTConfigSetExpClk(UART0_BASE, 16000000, 115200, (UART_CONFIG_WLEN_8 | UART_CONFIG_STOP_ONE | UART_CONFIG_PAR_NONE));
}

void ConfigureUART1(void){
    // Enable the GPIO Peripheral used by the UART.
    ROM_SysCtlPeripheralEnable(SYSCTL_PERIPH_GPIOB);
    // Enable UART1
    ROM_SysCtlPeripheralEnable(SYSCTL_PERIPH_UART1);

	// Set GPIO B0 and B1 as UART pins.
	ROM_GPIOPinConfigure(GPIO_PB0_U1RX);
	ROM_GPIOPinConfigure(GPIO_PB1_U1TX);
	ROM_GPIOPinTypeUART(GPIO_PORTB_BASE, GPIO_PIN_0 | GPIO_PIN_1);

    // Configure UART clock using UART utils. The above line does not work by itself to enable the UART.
    ROM_UARTClockSourceSet(UART1_BASE, UART_CLOCK_PIOSC);
    /*
    ROM_UARTConfigSetExpClk(UART1_BASE, 16000000, 115200, (UART_CONFIG_WLEN_8 | UART_CONFIG_STOP_ONE | UART_CONFIG_PAR_NONE));

	// Enable the UART1 interrupt.
	ROM_IntEnable(INT_UART1);

	// Set queue FIFO level interrupt.
	ROM_UARTFIFOLevelSet(UART1_BASE, UART_FIFO_TX1_8, UART_FIFO_RX1_8);

	// Two interrupt triggers: UART RX interrupt for a 1/8 of FIFO queue,
	// and Receive Timeout (RT) interrupt when 1/8 FIFO is not reach.
	ROM_UARTIntEnable(UART1_BASE, UART_INT_RX | UART_INT_RT);
	*/

	// Initialize the UART for console I/O.
	UARTStdioConfig(1, 115200, 16000000);
}

void ConfigureI2C3(void){
    // The I2C3 peripheral must be enabled before use.
    ROM_SysCtlPeripheralEnable(SYSCTL_PERIPH_I2C3);
    ROM_SysCtlPeripheralEnable(SYSCTL_PERIPH_GPIOD);
    // Configure the pin muxing for I2C3 functions on port D0 and D1.
    // This step is not necessary if your part does not support pin muxing.
    ROM_GPIOPinConfigure(GPIO_PD0_I2C3SCL);
    ROM_GPIOPinConfigure(GPIO_PD1_I2C3SDA);
    // Select the I2C function for these pins. This function will also configure the GPIO pins for
    // I2C operation, setting them to open-drain operation with weak pull-ups. Consult the data
    // sheet to see which functions are allocated per pin.
    ROM_GPIOPinTypeI2CSCL(GPIO_PORTD_BASE, GPIO_PIN_0);
    ROM_GPIOPinTypeI2C(GPIO_PORTD_BASE, GPIO_PIN_1);
}
