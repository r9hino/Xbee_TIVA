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
#include "utils/ustdlib.h"
#include "init_config.h"


// *****************************************************************************************
// Defines

// LED GPIOs
#define LED_RED GPIO_PIN_1
#define LED_BLUE GPIO_PIN_2
#define LED_GREEN GPIO_PIN_3

// Xbee Defines
#define MAX_FRAME_SIZE	      		   24
#define FRAME_TYPE_IDX		       		    3
#define RECEIVED_DATA_IDX		  		   15	// Idx for received data in ZB Receive Packet frame
// Especial data frame bytes
#define START_BYTE	 		  			 0x7E
#define ESCAPE_BYTE				         0x7D
#define XON_BYTE	 					 0x11
#define XOFF_BYTE	                     0x13
// Possible error type when receiving data packets.
#define NO_ERROR							0
#define CHECKSUM_FAILURE					1
#define PACKET_EXCEEDS_BYTE_ARRAY_LENGTH	2
#define UNEXPECTED_START_BYTE				3
// Escape macros definitions
#define ESCAPE_OFF 						    0
#define ESCAPE_ON  							1
#define ATAP	   							2
// Data frame types pr API ids.
#define ZB_TRANSMIT_REQUEST				 0x10
#define ZB_RECEIVE_PACKET				 0x90

// *****************************************************************************************
// Global variables

// Data frame structure. Save frame parameters.
typedef struct {
	uint8_t rxFrameData[MAX_FRAME_SIZE];	// Store UART1 received data.
	uint8_t pos;				// Store received byte position in frame.
	uint8_t lsbRxFrameLength;	// Store frame length. msb not used because frames are shorts.
	uint8_t frameType;
	bool rx_complete;			// True when all frame packets were successfully received.
	uint16_t rxChecksumTotal;	// Save frame checksum.
	uint8_t message[MAX_FRAME_SIZE - 16];	// Store message received from ZB Receive Packet frame.
	uint8_t messageIdx;			// Index for each message byte.
	uint8_t errorCode;
	bool escape;				// True when next frame byte will be the original escaped byte.
} tXbee;
tXbee tXbeeFrame;

// Store expected string to be received on message array in tXbee struct.
// Array size must be equal to message array size in tXbee struct.
// MAX_FRAME_DATA_SIZE - 16 = 8. This way dynamically memory allocation is avoided.
const uint8_t stringOn[MAX_FRAME_SIZE - 16] = {'o','n',0,0,0,0,0,0};
const uint8_t stringOff[MAX_FRAME_SIZE - 16] = {'o','f','f',0,0,0,0,0};

// Flag tell when new data has arrive in UART1 from xbee module.
bool flag_UART1_received_data = false;

// *****************************************************************************************
// Functions

// *****************************************************************************************
// Send string to PC from UART0 using uart.h driver.
void UART0Send(const uint8_t *stringBuffer){
	uint8_t stringLength = ustrlen((char *)stringBuffer);	// Get string length.
	while(stringLength--){
		ROM_UARTCharPut(UART0_BASE, *stringBuffer++);
	}
}

// *****************************************************************************************
void resetXbeeFrameData() {
	uint8_t i;
	for (i = 0; i<MAX_FRAME_SIZE; i++) {
		tXbeeFrame.rxFrameData[i] = 0;
		// Clear message array of length (MAX_FRAME_DATA_SIZE - 16).
		if(i < MAX_FRAME_SIZE - 16)   tXbeeFrame.message[i] = 0;
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

// *****************************************************************************************
// Send frame byte via xbee module.
uint8_t xbeeByteTx(uint8_t b, bool escapeMode) {
	if (escapeMode && (b == START_BYTE || b == ESCAPE_BYTE || b == XON_BYTE || b == XOFF_BYTE)) {
		ROM_UARTCharPut(UART1_BASE, ESCAPE_BYTE);
		ROM_UARTCharPut(UART1_BASE, b ^ 0x20);
		return b;
	}
	else {
		ROM_UARTCharPut(UART1_BASE, b);
		return b;
	}
}

// *****************************************************************************************
// Send data to coordinator via ZB Transmit Request frame.
void ZBTransmitRequest(const uint8_t *dataTxBuffer) {
	xbeeByteTx(START_BYTE, ESCAPE_OFF);									// 0. Start byte
	xbeeByteTx(0x00, ESCAPE_ON);										// 1. msb length

	// Get Tx data length.
	uint8_t dataTxLength = ustrlen((char *)dataTxBuffer);
	xbeeByteTx(14 + dataTxLength, ESCAPE_ON);							// 2. lsb length

	uint8_t checksum = 0;

	// Data frame and checksum start
	checksum += xbeeByteTx(ZB_TRANSMIT_REQUEST, ESCAPE_ON);				// 3. Frame type
	checksum += xbeeByteTx(0x00, ESCAPE_ON);							// 4. Frame Id number

	checksum += xbeeByteTx(0x00, ESCAPE_ON);							// 5. msb 64 address
	checksum += xbeeByteTx(0x00, ESCAPE_ON);							// 6. msb 64 address
	checksum += xbeeByteTx(0x00, ESCAPE_ON);							// 7. msb 64 address
	checksum += xbeeByteTx(0x00, ESCAPE_ON);							// 8. msb 64 address
	checksum += xbeeByteTx(0x00, ESCAPE_ON);   							// 9. lsb 64 address
	checksum += xbeeByteTx(0x00, ESCAPE_ON);							// 10. lsb 64 address
	checksum += xbeeByteTx(0x00, ESCAPE_ON);							// 11. lsb 64 address
	checksum += xbeeByteTx(0x00, ESCAPE_ON);							// 12. lsb 64 address

	checksum += xbeeByteTx(0xff, ESCAPE_ON);							// 13. msb 16 address
	checksum += xbeeByteTx(0xfe, ESCAPE_ON);							// 14. lsb 16 address

	checksum += xbeeByteTx(0x00, ESCAPE_ON);							// 15. Broadcast Radius
	checksum += xbeeByteTx(0x00, ESCAPE_ON);							// 16. Options

	// Transmit data buffer.
	uint8_t i;
	for (i=0; i<dataTxLength; i++) {
		checksum += xbeeByteTx(dataTxBuffer[i], ESCAPE_ON);				// 17. Start of data to send.
	}

	checksum = 0xff - checksum;
	xbeeByteTx(checksum, ESCAPE_ON);
}


// *****************************************************************************************
// ZB Receive Packet frame (0x90) handler using UART1 RX interrupt service.
void ZBReceivePacket(void) {
	uint8_t rxB;	// Received xbee byte.

	// Clear all old frame parameters when new frame arrive.
	if (tXbeeFrame.rx_complete || tXbeeFrame.errorCode) {
		resetXbeeFrameData();
	}

	while (ROM_UARTCharsAvail(UART1_BASE)) {
		rxB = (uint8_t)ROM_UARTCharGetNonBlocking(UART1_BASE);	// Be careful because UARTCharGetNonBlocking return -1 when there is not data.
		ROM_UARTCharPutNonBlocking(UART0_BASE, rxB);	// Write back to UART0 for PC displaying

		// Check if new packet start before previous packet completed. Discard previous packet and start over.
		if (tXbeeFrame.pos > 0 && rxB == START_BYTE) {
			tXbeeFrame.errorCode = UNEXPECTED_START_BYTE;
			return;
		}

		if ((tXbeeFrame.pos > 0) && (rxB == ESCAPE_BYTE)) {
			// Escape byte.  Next byte will be.
			tXbeeFrame.escape = true;
			continue;	// Re-execute while()
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
					tXbeeFrame.rxFrameData[tXbeeFrame.pos] = rxB;
					tXbeeFrame.pos++;
				}
				break;
			case 1:
				// msb length shouldn't be used because frames length will be short.
				tXbeeFrame.rxFrameData[tXbeeFrame.pos] = rxB;
				tXbeeFrame.pos++;
				break;
			case 2:
				// lsb length
				tXbeeFrame.lsbRxFrameLength = rxB;	//rxFrameLengthCalculation();
				tXbeeFrame.rxFrameData[tXbeeFrame.pos] = rxB;
				tXbeeFrame.pos++;
				break;
			case 3:
				tXbeeFrame.frameType = rxB;
				tXbeeFrame.rxFrameData[tXbeeFrame.pos] = rxB;
				tXbeeFrame.pos++;
				break;
			default:
				// Starts at fifth byte
				if (tXbeeFrame.pos > MAX_FRAME_SIZE) {
					// Exceed max size (Error code is 2).
					tXbeeFrame.errorCode = PACKET_EXCEEDS_BYTE_ARRAY_LENGTH;
					return;
				}

				// Store received data message from ZB Receive Packet frame (0x90)
				if ((tXbeeFrame.pos >= RECEIVED_DATA_IDX) && (tXbeeFrame.pos < (tXbeeFrame.lsbRxFrameLength + FRAME_TYPE_IDX)) && (tXbeeFrame.frameType == 0x90)) {
					// Fill array with data byte received.
					tXbeeFrame.message[tXbeeFrame.messageIdx] = rxB;
					tXbeeFrame.messageIdx++;
				}

				// Check if we are at the end of the packet.
				else if (tXbeeFrame.pos == (tXbeeFrame.lsbRxFrameLength + FRAME_TYPE_IDX)) {
					// rxChecksumTotal has included the checksum byte in it.
					if ((tXbeeFrame.rxChecksumTotal & 0xff) == 0xff) {
						tXbeeFrame.rx_complete = true;
						tXbeeFrame.errorCode = NO_ERROR;
						UART0Send((uint8_t *)"\n\r");	// With last byte received, send new line and return commands.

						if(!ustrcmp((char *)tXbeeFrame.message, (char *)stringOn)){
							ROM_GPIOPinWrite(GPIO_PORTF_BASE, LED_RED|LED_GREEN|LED_BLUE, LED_GREEN);
						}
						if(!ustrcmp((char *)tXbeeFrame.message, (char *)stringOff)){
							ROM_GPIOPinWrite(GPIO_PORTF_BASE, LED_RED|LED_GREEN|LED_BLUE, LED_RED);
						}
					}
					// Checksum failed.
					else {
						tXbeeFrame.errorCode = CHECKSUM_FAILURE;
						return;
					}
				}
				tXbeeFrame.rxFrameData[tXbeeFrame.pos] = rxB;
				tXbeeFrame.pos++;
		}
	}
}

// *****************************************************************************************
// UART1 RX interrupt Handler.
void UART1IntHandler(void){
	uint32_t ui32Status;

	// Get the interrrupts register status.
	ui32Status = ROM_UARTIntStatus(UART1_BASE, true);
	// Clear the asserted interrupts. Must be done early in handler.
	ROM_UARTIntClear(UART1_BASE, ui32Status);

	// Set flag for ZBReceivePacket function execution.
	flag_UART1_received_data = true;
}

//*****************************************************************************
// The interrupt handler for TIMER0 interrupt. It periodically transmit read sensor data.
void Timer0IntHandler(void) {
    // Clear the timer interrupt.
    ROM_TimerIntClear(TIMER0_BASE, TIMER_TIMA_TIMEOUT);

    // Use the flags to Toggle the LED for this timer.
    ROM_GPIOPinWrite(GPIO_PORTF_BASE, LED_RED|LED_GREEN|LED_BLUE, LED_BLUE);
    ROM_SysCtlDelay(ROM_SysCtlClockGet() / 1000);
    ROM_GPIOPinWrite(GPIO_PORTF_BASE, LED_RED|LED_GREEN|LED_BLUE, 0);

    ZBTransmitRequest((uint8_t *)"t20.5|h50.2|l180.5");
}

// Main ----------------------------------------------------------------------------------------------
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

	// Initialize xbee frame structure parameters.
	resetXbeeFrameData();

	while(1){
		// Enter when new packet has arrived in UART1.
		if(flag_UART1_received_data == true) {
			// Turn off the flag so ZBReceivePacket can be called again after a new interrupt in UART1 Rx.
			flag_UART1_received_data = false;
			ZBReceivePacket();
		}
		// Idea: use callback function in UARTHandler receive data and execute it when rx complete.
		// Then add actions like: ROM_GPIOPinWrite.
		// TO-DO: - Store sensor data in a structure and then send it to a string
		// 			creation routine to form "t20.5|h50.2|l180.5".

	}
}

