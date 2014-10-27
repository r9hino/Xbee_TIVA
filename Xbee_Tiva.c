//****************************************************************************************************
// Basic Xbee + Tiva example:
// UART1 configured to receive xbee data with 9600 baud rate.
// All data received by UART1 is sended to UART0 (115200) for PC displaying.
//****************************************************************************************************

#include <stdint.h>
#include <stdbool.h>
#include "inc/hw_ints.h"
#include "inc/hw_memmap.h"
#include "driverlib/gpio.h"
#include "driverlib/interrupt.h"
#include "driverlib/pin_map.h"
#include "driverlib/rom.h"
#include "driverlib/sysctl.h"
#include "driverlib/uart.h"
#include "utils/uartstdio.h"


// Functions -----------------------------------------------------------------------------------------
void UART1IntHandler(void){
	uint32_t ui32Status;
	uint8_t rxB;	// Received xbee byte.

	// Get the interrrupts register status.
	ui32Status = ROM_UARTIntStatus(UART1_BASE, true);
	// Clear the asserted interrupts. Must be done early in handler.
	ROM_UARTIntClear(UART1_BASE, ui32Status);

	while (ROM_UARTCharsAvail(UART1_BASE)) {
		rxB = (uint8_t)ROM_UARTCharGetNonBlocking(UART1_BASE);	// Be carefull because UARTCharGetNonBlocking return -1 when there is not data.

		// Write back to UART0 for PC displaying
		ROM_UARTCharPutNonBlocking(UART0_BASE, rxB);
		//ROM_UARTCharPut(UART0_BASE, 0xAB);
	}
}

void UARTSend(const uint8_t *pui8Buffer, uint32_t ui32Count){
	while(ui32Count--){
		ROM_UARTCharPut(UART0_BASE, *pui8Buffer++);
	}
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

	// Configure the UART for 115,200, 8-N-1 operation.
	//ROM_UARTConfigSetExpClk(UART0_BASE, ROM_SysCtlClockGet(), 115200, (UART_CONFIG_WLEN_8 | UART_CONFIG_STOP_ONE | UART_CONFIG_PAR_NONE));

    // Configure UART clock using UART utils. The above line does not work by itself to enable the UART.
	// The following two lines must be present. Why?
    UARTClockSourceSet(UART0_BASE, UART_CLOCK_PIOSC);
    UARTStdioConfig(0, 115200, 16000000);
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
	// The following two lines must be present. Why?
    UARTClockSourceSet(UART1_BASE, UART_CLOCK_PIOSC);
    UARTStdioConfig(1, 9600, 16000000);

	// Enable the UART1 interrupt.
	ROM_IntEnable(INT_UART1);
	ROM_UARTFIFOLevelSet(UART1_BASE, UART_FIFO_TX1_8, UART_FIFO_RX1_8);
	// Two interrupt triggers: UART RX interrupt for a 1/8 of FIFO queue,
	// and Receive Timeout (RT) interrupt when 1/8 FIFO is not reach.
	ROM_UARTIntEnable(UART1_BASE, UART_INT_RX | UART_INT_RT);
}

// Main ----------------------------------------------------------------------------------------------
int main(void){
	// Setup the system clock to run at 80 Mhz from PLL with crystal reference
	ROM_SysCtlClockSet(SYSCTL_SYSDIV_2_5|SYSCTL_USE_PLL|SYSCTL_XTAL_16MHZ|SYSCTL_OSC_MAIN);

	// Enable LEDS
	ROM_SysCtlPeripheralEnable(SYSCTL_PERIPH_GPIOF);
	ROM_GPIOPinTypeGPIOOutput(GPIO_PORTF_BASE, LED_RED|LED_BLUE|LED_GREEN);

	ConfigureUART0();
	ConfigureUART1();
	// Enable the UART0 interrupt.
	//ROM_IntEnable(INT_UART0);
	//ROM_UARTIntEnable(UART0_BASE, UART_INT_RX | UART_INT_RT);

	// Prompt for text to be entered.
	UARTSend((uint8_t *)"12345678901234567890\n\r", 22);

	// Enable interrupts
	ROM_IntMasterEnable();

	// Loop forever echoing data through the UART.
	while(1){

	}
}
