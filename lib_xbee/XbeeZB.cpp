/*
 * XbeeZB.cpp
 *
 *  Created on: 03-12-2014
 *      Author: r9hino
 */

#include <stdint.h>
#include <stdbool.h>
#include "inc/hw_memmap.h"
#include "driverlib/rom.h"
#include "lib_utils/ustdlib.h"
#include "lib_utils/uartstdio.h"
#include "lib_xbee/XbeeZB.h"

struct tXbee tXbeeFrame;

//**************************************************************************************************
// Constructor will initialize tXbee frame struct.
XbeeZB :: XbeeZB(){
	resetXbeeFrameInfo();
}

//**************************************************************************************************
// Clear all frame data.
void XbeeZB :: resetXbeeFrameInfo() {
	uint8_t i;
	for (i = 0; i<MAX_FRAME_SIZE; i++) {
		tXbeeFrame.rxFrameData[i] = 0;
		// Clear message array of length (MAX_FRAME_DATA_SIZE - 16).
		if(i < MAX_FRAME_SIZE - 16)   tXbeeFrame.message[i] = 0;
	}
	tXbeeFrame.pos = 0;
	tXbeeFrame.lsbRxFrameLength = 0;
	tXbeeFrame.frameType = 0;
	tXbeeFrame.rxChecksumTotal = 0;
	tXbeeFrame.messageIdx = 0;
	tXbeeFrame.errorCode = 0;
	tXbeeFrame.escape = false;
	tXbeeFrame.rxComplete = false;
	rxMsgPayloadComplete = false;
}

//**************************************************************************************************
// Send frame byte via UART to the xbee module.
uint8_t XbeeZB :: xbeeByteTx(uint8_t b, bool escapeMode) {
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

//**************************************************************************************************
// Send data to coordinator via ZB Transmit Request frame.
void XbeeZB :: ZBTransmitRequest(const uint8_t *payloadMsg) {
	xbeeByteTx(START_BYTE, ESCAPE_OFF);									// 0. Start byte
	xbeeByteTx(0x00, ESCAPE_ON);										// 1. msb length

	// Get Tx data length.
	uint8_t dataTxLength = ustrlen((char *)payloadMsg);
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
		checksum += xbeeByteTx(payloadMsg[i], ESCAPE_ON);				// 17. Start of data to send.
	}

	checksum = 0xff - checksum;
	xbeeByteTx(checksum, ESCAPE_ON);
}


//**************************************************************************************************
// ZB Receive Packet frame (0x90) handler executed with UART RX interrupt.
void XbeeZB :: ZBReceivePacket(void){
	uint8_t rxB;	// Received xbee byte.

	// Clear all old frame parameters when new frame arrive.
	if(tXbeeFrame.rxComplete || tXbeeFrame.errorCode){
		resetXbeeFrameInfo();
	}

	// Check if Rx ring buffer has some data packet.
	while(UARTRxBytesAvail()){
		rxB = (uint8_t)UARTgetc();						// Get character stored in Rx ring buffer.
		ROM_UARTCharPutNonBlocking(UART0_BASE, rxB);	// Write back to UART0 for PC displaying

		// Check if new packet start before previous packet completed. Discard previous packet and start over.
		if(tXbeeFrame.pos > 0 && rxB == START_BYTE) {
			tXbeeFrame.errorCode = UNEXPECTED_START_BYTE;
			return;
		}

		if((tXbeeFrame.pos > 0) && (rxB == ESCAPE_BYTE)){
			// Escape byte.  Next byte will be.
			tXbeeFrame.escape = true;
			continue;	// Re-execute while()
		}

		// If previous byte was an escape byte, then next byte must be XOR'ed.
		if(tXbeeFrame.escape == true){
			rxB = 0x20 ^ rxB;
			tXbeeFrame.escape = false;
		}

		// Checksum includes all bytes after frame type byte.
		if(tXbeeFrame.pos >= FRAME_TYPE_IDX){
			tXbeeFrame.rxChecksumTotal += rxB;
		}

		switch(tXbeeFrame.pos){
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
						tXbeeFrame.rxComplete = true;
						rxMsgPayloadComplete = true;	// Payload message is complete. Now xbee_cmdline can parse sended cmd.
						tXbeeFrame.errorCode = NO_ERROR;
						ROM_UARTCharPutNonBlocking(UART0_BASE, '\n');
						ROM_UARTCharPutNonBlocking(UART0_BASE, '\r');
						return;
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

//**************************************************************************************************
// Get received message payload from frame packet.
uint8_t* XbeeZB :: getRxMsgPayload(){
	return tXbeeFrame.message;
}

//**************************************************************************************************
// Check if received data is complete and ready to be used and parsed. Return true when is complete.
bool XbeeZB :: isRxComplete(){
	return  tXbeeFrame.rxComplete;
}
//**************************************************************************************************
// It check if received payload message is complete.
/*bool XbeeZB :: isRxMsgPayloadComplete(){
	return tXbeeFrame.rxMsgPayloadComplete;
}*/

