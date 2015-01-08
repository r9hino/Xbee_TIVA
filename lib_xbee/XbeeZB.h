/*
 * XbeeZB.h
 *  Created on: 03-12-2014
 *      Author: r9hino
 */

#ifndef XBEEZB_H_
#define XBEEZB_H_


// Xbee Defines
#define MAX_FRAME_SIZE	      		      255
#define FRAME_TYPE_IDX		       		    3	// Position index of frame type byte in frame packet.
#define RECEIVED_DATA_IDX		  		   15	// Idx for received data in ZB Receive Packet frame.
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
// TX frame types API ids.
#define AT_COMMAND								0x08	// Local
#define AT_COMMAND_QUEUE_PARAMETER			    0x09	// Local
#define ZB_TRANSMIT_REQUEST				 		0x10	// Remote
#define EXPLICIT_ADDRESSING_ZB_COMMAND_FRAME	0x11	// Remote
#define REMOTE_AT_COMMAND_REQUEST		 		0x17	// Remote
#define CREATE_SOURCE_ROUTE				 		0x21	// Local
// RX frame types API ids.
#define MODEM_STATUS					 		0x8A	// Local and Remote?
#define ZB_TRANSMIT_STATUS						0x8B	// Remote
#define AT_COMMAND_RESPONSE				 		0x88	// Local and Remote?
#define ZB_RECEIVE_PACKET				 		0x90	// Remote
#define ZB_EXPLICIT_RX_INDICATOR				0x91	// Remote
#define ZB_IO_DATA_SAMPLE_RX_INDICATOR			0x92	// Remote
#define NODE_IDENTIFICATION_INDICATOR			0x95	// Remote
#define REMOTE_COMMAND_RESPONSE					0x97	// Remote
#define ROUTE_RECORD_INDICATOR					0xA1	// Remote
#define MANY_TO_ONE_ROUTE_REQUEST_INDICATOR		0xA3	// Remote



// Data frame structure. Save frame parameters.
struct tXbee{
	uint8_t rxFrameData[MAX_FRAME_SIZE];	// Store frame packet data received from UART1.
	uint8_t pos;							// Store received byte position in frame.
	uint8_t lsbRxFrameLength;				// Store frame length. msb not used because frames are shorts.
	uint8_t frameType;
	uint16_t rxChecksumTotal;				// Save frame checksum.
	uint8_t message[MAX_FRAME_SIZE - 16];	// Store message received from ZB Receive Packet frame.
	uint8_t messageIdx;						// Index for each message byte.
	uint8_t errorCode;
	bool escape;							// True when next frame byte will be the original escaped byte.
	bool rxComplete;						// True when all frame bytes were successfully received.
};



class XbeeZB {
public:
	//**************************************************************************************************
	// Constructor will initialize tXbee frame struct.
	XbeeZB();

	//**************************************************************************************************
	// Clear all frame data.
	void resetXbeeFrameInfo(void);

	//**************************************************************************************************
	// Send frame byte via UART to the xbee module.
	uint8_t xbeeByteTx(uint8_t b, bool escapeMode);

	//**************************************************************************************************
	// Send data to coordinator via ZB Transmit Request frame.
	void ZBTransmitRequest(const uint8_t *payloadMsg);

	//**************************************************************************************************
	// ZB Receive Packet frame (0x90) handler executed with UART RX interrupt.
	void ZBReceivePacket(void);

	//**************************************************************************************************
	// Get received message payload from frame packet.
	uint8_t* getRxMsgPayload(void);

	//**************************************************************************************************
	// It check if received data is complete and ready to be used and parsed.
	bool isRxComplete(void);

	//**************************************************************************************************
	// It check if received payload message is complete.
	//bool isRxMsgPayloadComplete(void);

	//**************************************************************************************************
	// Variables
	bool rxMsgPayloadComplete;	// True when payload message is completelly received.
};


#endif /* XBEEZB_H_ */
