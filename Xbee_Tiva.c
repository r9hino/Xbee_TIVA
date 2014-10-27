//****************************************************************************************************
// Basic xbee + Tiva example:
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


// Defines *******************************************************************************************
#define LED_RED GPIO_PIN_1
#define LED_BLUE GPIO_PIN_2
#define LED_GREEN GPIO_PIN_3

// Xbee Defines **************************************************************************************
#define MAX_FRAME_DATA_SIZE	      24
#define FRAME_TYPE_IDX		       3
#define RECEIVED_DATA_IDX		  15	// Idx for received data in ZB Receive Packet frame
// Especial data frame bytes
#define START_BYTE	 		  	0x7E
#define ESCAPE_BYTE				0x7D
// Possible error type when receiving data packets.
#define NO_ERROR							0
#define CHECKSUM_FAILURE					1
#define PACKET_EXCEEDS_BYTE_ARRAY_LENGTH	2
#define UNEXPECTED_START_BYTE				3


// Global variables **********************************************************************************

// Data frame structure. Save frame parameters.
typedef struct {
	uint8_t rxFrameData[MAX_FRAME_DATA_SIZE];	// Store UART1 received data.
	uint8_t pos;				// Store received byte position in frame.
	uint8_t lsbRxFrameLength;	// Store frame length. msb not used because frames are shorts.
	uint8_t frameType;
	bool rx_complete;			// True when all frame packets were successfully received.
	uint16_t rxChecksumTotal;	// Save frame checksum.
	uint8_t message[MAX_FRAME_DATA_SIZE - 16];	// Store message received from ZB Receive Packet frame.
	uint8_t messageIdx;			// Index for each message byte.
	uint8_t errorCode;
	bool escape;				// True when next frame byte will be the original escaped byte.
} tXbee;
volatile tXbee tXbeeFrame;

void resetXbeeFrameData() {
	uint8_t i;
	for (i = 0; i<MAX_FRAME_DATA_SIZE; i++) {
		tXbeeFrame.rxFrameData[i] = 0;
		// Clear message array of length (MAX_FRAME_DATA_SIZE - 16).
		if(i < MAX_FRAME_DATA_SIZE - 16)   tXbeeFrame.message[i] = 0;
	}
	tXbeeFrame.pos = 0;
	tXbeeFrame.lsbRxFrameLength = 0;
	tXbeeFrame.frameType = 0;
	tXbeeFrame.rx_complete = false;
	tXbeeFrame.rxChecksumTotal = 0;
	tXbeeFrame.messageIdx = 0;
	tXbeeFrame.errorCode = 0;
	tXbeeFrame.escape = false;
}


// Functions -----------------------------------------------------------------------------------------
void UART1IntHandler(void){
	uint32_t ui32Status;
	uint8_t rxB;	// Received xbee byte.

	// Get the interrrupts register status.
	ui32Status = ROM_UARTIntStatus(UART1_BASE, true);
	// Clear the asserted interrupts. Must be done early in handler.
	ROM_UARTIntClear(UART1_BASE, ui32Status);

	// Clear all old frame parameters when new frame arrive.
	if (tXbeeFrame.rx_complete || tXbeeFrame.errorCode) {
		resetXbeeFrameData();
	}

	while (ROM_UARTCharsAvail(UART1_BASE)) {
		rxB = (uint8_t)ROM_UARTCharGetNonBlocking(UART1_BASE);	// Be careful because UARTCharGetNonBlocking return -1 when there is not data.

		if (tXbeeFrame.pos > 0 && rxB == START_BYTE) {
			// new packet start before previous packeted completed -- discard previous packet and start over
			tXbeeFrame.errorCode = UNEXPECTED_START_BYTE;
		    return;
		}

		if ((tXbeeFrame.pos > 0) && (rxB == ESCAPE_BYTE)) {
			// Escape byte.  Next byte will be.
			tXbeeFrame.escape = true;
			continue;
		}

		// If previous byte was an escape byte, then next byte must be XOR'ed.
		if (tXbeeFrame.escape == true) {
			rxB = 0x20 ^ rxB;
			tXbeeFrame.escape = false;
		}

		// Checksum includes all bytes after frame type byte.
		if (tXbeeFrame.pos >= FRAME_TYPE_IDX) {
			tXbeeFrame.rxChecksumTotal += rxB;
		}

		switch(tXbeeFrame.pos) {
			case 0:
				if (rxB == START_BYTE) {
				}
				break;
			case 1:
				// msb length shouldn't be used because frames length will be short.
				break;
			case 2:
				// lsb length
				tXbeeFrame.lsbRxFrameLength = rxB;//rxFrameLengthCalculation();
				break;
			case 3:
				tXbeeFrame.frameType = rxB;
				//_serialPrint->print(b, HEX);
				//_serialPrint->print(" ");
				break;
			default:
				// Starts at fifth byte
				if (tXbeeFrame.pos > MAX_FRAME_DATA_SIZE) {
					// Exceed max size.
					//_serialPrint->println("Error 2");
					tXbeeFrame.errorCode = PACKET_EXCEEDS_BYTE_ARRAY_LENGTH;
					return;
				}

				// Store received data message from ZB Receive Packet frame (0x90)
				if (tXbeeFrame.frameType == 0x90) {
					if ((tXbeeFrame.pos >= RECEIVED_DATA_IDX) && (tXbeeFrame.pos < (tXbeeFrame.lsbRxFrameLength + FRAME_TYPE_IDX))) {
						tXbeeFrame.message[tXbeeFrame.messageIdx] = rxB;
						tXbeeFrame.messageIdx++;
					}
				}

				// Check if we are at the end of the packet.
				if (tXbeeFrame.pos == (tXbeeFrame.lsbRxFrameLength + FRAME_TYPE_IDX)) {
					// rxChecksumTotal has included the checksum byte in it.
					if ((tXbeeFrame.rxChecksumTotal & 0xff) == 0xff) {
						tXbeeFrame.rx_complete = true;
						tXbeeFrame.errorCode = NO_ERROR;
					}
				}
		}

		tXbeeFrame.rxFrameData[tXbeeFrame.pos] = rxB;
		tXbeeFrame.pos++;

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

    // Configure UART clock using UART utils. The above line does not work by itself to enable the UART.
	// The following two lines must be present. Why?
    ROM_UARTClockSourceSet(UART0_BASE, UART_CLOCK_PIOSC);
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
    ROM_UARTClockSourceSet(UART1_BASE, UART_CLOCK_PIOSC);
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

	resetXbeeFrameData();
	// Loop forever echoing data through the UART.
	while(1){

	}
}
