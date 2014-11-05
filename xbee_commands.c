//*****************************************************************************
// rgb_commands.c - Command line functionality implementation
// This is part of revision 2.0.1.11577 of the EK-TM4C123GXL Firmware Package.
//*****************************************************************************

#include <stdint.h>
#include <stdbool.h>
#include "inc/hw_types.h"
#include "utils/ustdlib.h"
#include "utils/cmdline.h"
#include "xbee_commands.h"
#include "ctype.h"
#include "rgb_driver.h"

//*****************************************************************************
// Table of valid command strings, callback functions and help messages.  This
// is used by the cmdline module.
//*****************************************************************************
tCmdLineEntry g_psCmdTable[] =
{
    {"help", CMD_help, " : Display list of commands" },
    {"on", CMD_set_on, " : Turn on"},
    {"off", CMD_set_off, " : Turn off"},
};

//*****************************************************************************
// Print the help strings for all commands.
int CMD_help(int argc, char **argv){
    return (0);
}

//*****************************************************************************

int CMD_set_on(int argc, char **argv){
	if(argc == 1){
		ROM_GPIOPinWrite(GPIO_PORTF_BASE, LED_RED|LED_GREEN|LED_BLUE, LED_GREEN);
	}
	return 0;
}

int CMD_set_off(int argc, char **argv){
	if(argc == 1){
		ROM_GPIOPinWrite(GPIO_PORTF_BASE, LED_RED|LED_GREEN|LED_BLUE, LED_RED);
	}
	return 0;
}
