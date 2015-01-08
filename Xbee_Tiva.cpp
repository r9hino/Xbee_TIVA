//****************************************************************************************************
// Xbee_Tiva.c		Author: Philippe Ilharreguy
//
// Basic xbee + Tiva TM4C123G example:
// UART1 configured to receive xbee data with 9600 baud rate.
// All data received by UART1 is sended to UART0 (115200) for PC displaying.
//****************************************************************************************************

#include <stdbool.h>
#include <stdint.h>
#include <string.h>
#include <math.h>

#include "inc/hw_memmap.h"
#include "inc/hw_ints.h"
#include "driverlib/gpio.h"
#include "driverlib/pin_map.h"
#include "driverlib/rom.h"
#include "driverlib/sysctl.h"
#include "driverlib/timer.h"

#include "lib_utils/ustdlib.h"
#include "lib_utils/uartstdio.h"
#include "lib_xbee/XbeeZB.h"
#include "lib_xbee/xbee_data_parser.h"

#include "sensor/i2cm_drv.h"
#include "sensor/hw_bmp180.h"
#include "sensor/bmp180.h"
#include "sensor/hw_sht21.h"
#include "sensor/sht21.h"
#include "sensor/hw_isl29023.h"
#include "sensor/isl29023.h"

#include "configperiph.h"

//**************************************************************************************************
// Defines

// LED GPIOs
#define LED_RED 		  GPIO_PIN_1
#define LED_BLUE 		  GPIO_PIN_2
#define LED_GREEN 		  GPIO_PIN_3

// The system tick rate expressed both as ticks per second and a millisecond period.
#define TIMER0_PERIOD       	  45		// Timer0 period in seconds.
#define PRINT_INTERVAL_SEC         5		// Time interval in second for printing data to console.

// Define I2C Devices Addresses.
#define BMP180_I2C_ADDRESS      0x77
#define SHT21_I2C_ADDRESS  		0x40
#define ISL29023_I2C_ADDRESS    0x44

//**************************************************************************************************
// Global variables
XbeeZB XbeeZB;
char g_cZBTxReqSensorsString[48];		// Store string with all sensor values "t20.5|h50.2|l180.5".

tI2CMInstance g_sI2CInst;				// Global instance structure for the I2C master driver.

tBMP180 g_sBMP180Inst;					// Global instance structure for the BMP180 sensor driver.
tSHT21 g_sSHT21Inst;					// Global instance structure for the SHT21 sensor driver.
tISL29023 g_sISL29023Inst;				// Global instance structure for the ISL29023 sensor driver.

volatile uint_fast8_t g_vui8DataFlag;	// Global new data flag to alert main that BMP180 data is ready.
volatile uint_fast8_t g_vui8ErrorFlag;	// Global new error flag to store the error condition if encountered.

typedef struct{
	float fTemp;				// Temperature.
	char cTempString[11];

	float fPres;				// Pressure.
	char cPresString[11];

	float fHum;					// Humidity.
	char cHumString[11];

	float fLight;				// Ambient Light.
	char cLightString[11];
}
tSensorsValues;
tSensorsValues g_sSensorValues;		// Store sensors values.

//**************************************************************************************************
// Functions Prototypes
extern "C" void Timer0IntHandler(void);
extern "C" void SensorI2CIntHandler(void);

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

// Convert float to string with 2 decimal digit.
void floatToString(float floatValue, char destinationBuffer[]){
	int32_t i32IntegerPart;
	int32_t i32FractionPart;
	char integerPartStr[7];
	char fractionPartStr[2];

	i32IntegerPart = (int32_t)floatValue;
	i32FractionPart = (int32_t)(floatValue * 100.0f);
	i32FractionPart = i32FractionPart - (i32IntegerPart * 100);
	if(i32FractionPart < 0){
		i32FractionPart *= -1;
	}

	usprintf(integerPartStr, "%d", i32IntegerPart);
	usprintf(fractionPartStr, "%d", i32FractionPart);

	strcpy(destinationBuffer, integerPartStr);	// Automatically clean destinationBuffer string.
	strcat(destinationBuffer, ".");
	strcat(destinationBuffer, fractionPartStr);
}

//***************************************************************************************************
// SHT21, BMP180, ISL29023 sensors callback function. Called at the end of SHT21, BMP180, ISL29023
// sensor driver transactions. This is called from I2C interrupt context. Therefore, we just set a
// flag and let main do the bulk of the computations and display.
void SensorAppCallback(void* pvCallbackData, uint_fast8_t ui8Status){
	// If the transaction succeeded set the data flag to indicate to application that this
	// transaction is complete and data may be ready.
    if(ui8Status == I2CM_STATUS_SUCCESS){
        g_vui8DataFlag = 1;
    }
    g_vui8ErrorFlag = ui8Status;  // Store the most recent status in case it was an error condition.
}

//***************************************************************************************************
// Called by the NVIC as a result of I2C3 Interrupt. I2C3 is the I2C connection to SHT21, BMP180.
void SensorI2CIntHandler(void){
    // Pass through to the I2CM interrupt handler provided by sensor library. This is required to be
    // at app level so that I2CMIntHandler can receive the instance structure pointer as an argument.
    I2CMIntHandler(&g_sI2CInst);
}

//**************************************************************************************************
// The interrupt handler for TIMER0 interrupt. It periodically transmit read sensor data.
void Timer0IntHandler(void){
    // Clear the timer interrupt.
    ROM_TimerIntClear(TIMER0_BASE, TIMER_TIMA_TIMEOUT);

    // Use the flags to Toggle the LED for this timer.
    ROM_GPIOPinWrite(GPIO_PORTF_BASE, LED_RED|LED_GREEN|LED_BLUE, LED_BLUE);
    ROM_SysCtlDelay(ROM_SysCtlClockGet() / 1000);
    ROM_GPIOPinWrite(GPIO_PORTF_BASE, LED_RED|LED_GREEN|LED_BLUE, 0);

    strcpy(g_cZBTxReqSensorsString, "t");
    strcat(g_cZBTxReqSensorsString, g_sSensorValues.cTempString);
    strcat(g_cZBTxReqSensorsString, "|");
    strcat(g_cZBTxReqSensorsString, "p");
	strcat(g_cZBTxReqSensorsString, g_sSensorValues.cPresString);
	strcat(g_cZBTxReqSensorsString, "|");
	strcat(g_cZBTxReqSensorsString, "h");
	strcat(g_cZBTxReqSensorsString, g_sSensorValues.cHumString);
	strcat(g_cZBTxReqSensorsString, "|");
	strcat(g_cZBTxReqSensorsString, "l");
	strcat(g_cZBTxReqSensorsString, g_sSensorValues.cLightString);

	XbeeZB.ZBTransmitRequest((uint8_t *)g_cZBTxReqSensorsString);	// Send sensor value to gateway.

	UART0Send((uint8_t *)g_cZBTxReqSensorsString);
	UART0Send((uint8_t *)"\n\r");
}

//**************************************************************************************************
int main(void){
	// Setup the system clock to run at 80 Mhz from PLL with crystal reference
	ROM_SysCtlClockSet(SYSCTL_SYSDIV_2_5|SYSCTL_USE_PLL|SYSCTL_XTAL_16MHZ|SYSCTL_OSC_MAIN);

	// Enable LEDS
	ROM_SysCtlPeripheralEnable(SYSCTL_PERIPH_GPIOF);
	ROM_GPIOPinTypeGPIOOutput(GPIO_PORTF_BASE, LED_RED|LED_BLUE|LED_GREEN);

	ConfigureTimer0(TIMER0_PERIOD);
	ConfigureUART0();
	ConfigureUART1();
	ConfigureI2C3();

	// Prompt for text to be entered.
	UART0Send((uint8_t *)"\n\rWSN Tiva TM4C123G + Xbee Module\n\r");

    // Enable interrupts to the processor.
    ROM_IntMasterEnable();

    // Initialize the I2C3 peripheral.
    I2CMInit(&g_sI2CInst, I2C3_BASE, INT_I2C3, 0xff, 0xff, ROM_SysCtlClockGet());


    // Initialize the BMP180.
    BMP180Init(&g_sBMP180Inst, &g_sI2CInst, BMP180_I2C_ADDRESS, SensorAppCallback, &g_sBMP180Inst);
    // Wait for initialization callback to indicate reset request is complete.
    while(g_vui8DataFlag == 0){
        // Wait for I2C Transactions to complete.
    }
    g_vui8DataFlag = 0;		// Reset the data ready flag
    ROM_SysCtlDelay(ROM_SysCtlClockGet()/(30*3));


    // Initialize the SHT21.
    SHT21Init(&g_sSHT21Inst, &g_sI2CInst, SHT21_I2C_ADDRESS, SensorAppCallback, &g_sSHT21Inst);
    // Wait for initialization callback to indicate reset request is complete.
	while(g_vui8DataFlag == 0){
		// Wait for I2C Transactions to complete.
	}
	g_vui8DataFlag = 0;		// Reset the data ready flag
	ROM_SysCtlDelay(ROM_SysCtlClockGet()/(30*3));


	// Initialize the ISL29023 Driver.
	ISL29023Init(&g_sISL29023Inst, &g_sI2CInst, ISL29023_I2C_ADDRESS, SensorAppCallback, &g_sISL29023Inst);
    // Wait for initialization callback to indicate reset request is complete.
	while(g_vui8DataFlag == 0){
		// Wait for I2C Transactions to complete.
	}
	// Configure the ISL29023 to measure ambient light continuously. Set a 8
	// sample persistence before the INT pin is asserted. Clears the INT flag.
	// Persistence setting of 8 is sufficient to ignore camera flashes.
	uint8_t ui8Mask = (ISL29023_CMD_I_OP_MODE_M | ISL29023_CMD_I_INT_PERSIST_M | ISL29023_CMD_I_INT_FLAG_M);
	ISL29023ReadModifyWrite(&g_sISL29023Inst, ISL29023_O_CMD_I, ~ui8Mask,
							(ISL29023_CMD_I_OP_MODE_ALS_CONT | ISL29023_CMD_I_INT_PERSIST_8),
							SensorAppCallback, &g_sISL29023Inst);
    // Wait for initialization callback to indicate reset request is complete.
	while(g_vui8DataFlag == 0){
		// Wait for I2C Transactions to complete.
	}
	ROM_SysCtlDelay(ROM_SysCtlClockGet()/(30*3));

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

		// Read the data from the BMP180 over I2C. This command starts a temperature measurement.
		// Then polls until temperature is ready. Then automatically starts a pressure
		// measurement and polls for that to complete. When both measurement are complete and in
		// the local buffer then the application callback is called from the I2C interrupt context.
		// Polling is done on I2C interrupts allowing processor to continue doing other tasks as needed.
		BMP180DataRead(&g_sBMP180Inst, SensorAppCallback, &g_sBMP180Inst);
		while(g_vui8DataFlag == 0){
			// Wait for the new data set to be available.
		}
		g_vui8DataFlag = 0;		// Reset the data ready flag.
		// Get a local copy of the latest temperature data in float format.
		BMP180DataTemperatureGetFloat(&g_sBMP180Inst, &g_sSensorValues.fTemp);
		floatToString(g_sSensorValues.fTemp, g_sSensorValues.cTempString);
		// Get a local copy of the latest air pressure data in float format.
		BMP180DataPressureGetFloat(&g_sBMP180Inst, &g_sSensorValues.fPres);
		floatToString(g_sSensorValues.fPres, g_sSensorValues.cPresString);


		// Write the command to start a humidity measurement.
		SHT21Write(&g_sSHT21Inst, SHT21_CMD_MEAS_RH, g_sSHT21Inst.pui8Data, 0, SensorAppCallback, &g_sSHT21Inst);
		while(g_vui8DataFlag == 0){
			// Wait for the new data set to be available.
		}
		g_vui8DataFlag = 0;		// Reset the data ready flag.
		// Wait 33 milliseconds before attempting to get the result. Datasheet
		// claims this can take as long as 29 milliseconds.
		ROM_SysCtlDelay(ROM_SysCtlClockGet() / (30 * 3));
		// Get the raw data from the sensor over the I2C bus.
		SHT21DataRead(&g_sSHT21Inst, SensorAppCallback, &g_sSHT21Inst);
		while(g_vui8DataFlag == 0){
			// Wait for the new data set to be available.
		}
		g_vui8DataFlag = 0;		// Reset the data ready flag.
		// Get a copy of the most recent raw data in floating point format.
		SHT21DataHumidityGetFloat(&g_sSHT21Inst, &g_sSensorValues.fHum);
		g_sSensorValues.fHum *= 100.0f;		// Multiply by 100 to return percentage.
		floatToString(g_sSensorValues.fHum, g_sSensorValues.cHumString);


		// Go get the latest data from the sensor.
		ISL29023DataRead(&g_sISL29023Inst, SensorAppCallback, &g_sISL29023Inst);
		while(g_vui8DataFlag == 0){
			// Wait for the new data set to be available.
		}
		g_vui8DataFlag = 0;		// Reset the data ready flag.
		// Get a local floating point copy of the latest light data
		ISL29023DataLightVisibleGetFloat(&g_sISL29023Inst, &g_sSensorValues.fLight);
		floatToString(g_sSensorValues.fLight, g_sSensorValues.cLightString);
	}
}

