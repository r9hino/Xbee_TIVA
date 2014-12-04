//****************************************************************************************************
// Xbee_Tiva.c		Author: Philippe Ilharreguy
//
// Basic xbee + Tiva TM4C123G example:
// UART1 configured to receive xbee data with 9600 baud rate.
// All data received by UART1 is sended to UART0 (115200) for PC displaying.
//****************************************************************************************************

#include <stdint.h>
#include <stdbool.h>
#include "inc/hw_memmap.h"
#include "driverlib/gpio.h"
#include "driverlib/rom.h"
#include "driverlib/sysctl.h"
#include "driverlib/timer.h"
#include "lib_utils/ustdlib.h"
#include "lib_utils/uartstdio.h"
#include "lib_xbee/XbeeZB.h"
#include "lib_xbee/xbee_data_parser.h"
#include "configperiph.h"

//**************************************************************************************************
// Defines

// LED GPIOs
#define LED_RED GPIO_PIN_1
#define LED_BLUE GPIO_PIN_2
#define LED_GREEN GPIO_PIN_3

//**************************************************************************************************
// Global variables
XbeeZB XbeeZB;


//**************************************************************************************************
// Functions Prototypes
extern "C" void Timer0IntHandler(void);


/******************************************************************************
 * The error routine that is called if the driver library encounters an error.
 *****************************************************************************/
#ifdef DEBUG
void __error__(char *pcFilename, uint32_t ui32Line){
}
#endif

//**************************************************************************************************
// Send string to PC from UART0 using uart.h driver.
void UART0Send(const uint8_t *stringBuffer){
	uint8_t stringLength = ustrlen((char *)stringBuffer);	// Get string length.
	while(stringLength--){
		ROM_UARTCharPut(UART0_BASE, *stringBuffer++);
	}
}

//**************************************************************************************************
// The interrupt handler for TIMER0 interrupt. It periodically transmit read sensor data.
void Timer0IntHandler(void) {
    // Clear the timer interrupt.
    ROM_TimerIntClear(TIMER0_BASE, TIMER_TIMA_TIMEOUT);

    // Use the flags to Toggle the LED for this timer.
    ROM_GPIOPinWrite(GPIO_PORTF_BASE, LED_RED|LED_GREEN|LED_BLUE, LED_BLUE);
    ROM_SysCtlDelay(ROM_SysCtlClockGet() / 1000);
    ROM_GPIOPinWrite(GPIO_PORTF_BASE, LED_RED|LED_GREEN|LED_BLUE, 0);

    XbeeZB.ZBTransmitRequest((uint8_t *)"t20.5|h50.2|l180.5");
}

//**************************************************************************************************
int main(void){
	// Setup the system clock to run at 80 Mhz from PLL with crystal reference
	ROM_SysCtlClockSet(SYSCTL_SYSDIV_2_5|SYSCTL_USE_PLL|SYSCTL_XTAL_16MHZ|SYSCTL_OSC_MAIN);

	// Enable LEDS
	ROM_SysCtlPeripheralEnable(SYSCTL_PERIPH_GPIOF);
	ROM_GPIOPinTypeGPIOOutput(GPIO_PORTF_BASE, LED_RED|LED_BLUE|LED_GREEN);

	ConfigureTimer0();
	ConfigureUART0();
	ConfigureUART1();

	// Prompt for text to be entered.
	UART0Send((uint8_t *)"\n\rWSN Tiva TM4C123G + Xbee Module\n\r");

	// Enable interrupts
	ROM_IntMasterEnable();

	// Store return value from xbeeCmdLineProcess
	int8_t i32CommandStatus;

	while(1){
		// Enter when new packet has arrived in UART1.
		if(UARTRxBytesAvail()){
			XbeeZB.ZBReceivePacket();
		}

		// Enter when xbee data message is retrieved from ZBReceivePacket frame.
		if(XbeeZB.rxMsgPayloadComplete == true){
			// Turn off the flag in order to enter just one time here.
			XbeeZB.rxMsgPayloadComplete = false;

			// Pass xbee data message to command line processor.
			i32CommandStatus = xbeeCmdLineProcess(XbeeZB.getRxMsgPayload());

			// Handle the case of bad command.
	        if(i32CommandStatus == CMDLINE_BAD_CMD){
	        	UART0Send((uint8_t *)"Bad command!\n\r");
	        }

	        // Handle the case of too many arguments.
	        else if(i32CommandStatus == CMDLINE_TOO_MANY_ARGS){
	        	UART0Send((uint8_t *)"Too many arguments for xbee command processor!\n\r");
	        }
		}
		// TO-DO: - Store sensor data in a structure and then send it to a string
		// 			creation routine to form "t20.5|h50.2|l180.5".

	}
}

