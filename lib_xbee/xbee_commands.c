//*****************************************************************************
// xbee_commands.c - Command line functionality implementation for xbee data
// 					 messages received.
// argv - Store string
//*****************************************************************************

#include <stdint.h>
#include <stdbool.h>
#include "driverlib/gpio.h"
#include "driverlib/rom.h"
#include "inc/hw_memmap.h"
#include "lib_xbee/xbee_data_parser.h"
#include "lib_xbee/xbee_commands.h"

//*****************************************************************************
// Table of valid command strings, callback functions and help messages.  This
// is used by the cmdline module.
//*****************************************************************************
tCmdLineEntry g_psCmdTable[] = {
    {"help", CMD_help, " : Display list of commands" },
    {"on", CMD_set_on, " : Turn on"},
    {"off", CMD_set_off, " : Turn off"},
    {"aaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaa", CMD_set_test, " : Test data payload"},
};

// argc is the number of arguments.
// argv is an array with the function's string parameters.

//*****************************************************************************
// Print the help strings for all commands.
int8_t CMD_help(uint8_t argc, uint8_t **argv) {
    return 0;
}

//*****************************************************************************

int8_t CMD_set_on(uint8_t argc, uint8_t **argv) {
	if(argc == 1){
		// Turn green LED on.
		ROM_GPIOPinWrite(GPIO_PORTF_BASE, GPIO_PIN_1|GPIO_PIN_3|GPIO_PIN_2, GPIO_PIN_3);
	}
	return 0;
}

int8_t CMD_set_off(uint8_t argc, uint8_t **argv) {
	if(argc == 1){
		// Turn red LED on.
		ROM_GPIOPinWrite(GPIO_PORTF_BASE, GPIO_PIN_1|GPIO_PIN_3|GPIO_PIN_2, GPIO_PIN_1);
	}
	return 0;
}

int8_t CMD_set_test(uint8_t argc, uint8_t **argv) {
	if(argc == 1){
		// Turn red LED on.
		ROM_GPIOPinWrite(GPIO_PORTF_BASE, GPIO_PIN_1|GPIO_PIN_3|GPIO_PIN_2, GPIO_PIN_2);
	}
	return 0;
}
