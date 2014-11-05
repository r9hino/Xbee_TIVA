//*****************************************************************************
// xbee_commands.h - Prototypes functions that handles commands sended
// 					 from server to xbee


#ifndef __XBEE_COMMANDS_H__
#define __XBEE_COMMANDS_H__

//*****************************************************************************
// Defines for the command line argument parser provided as a standard part of TivaWare.
// Xbee application uses the command parser to extend functionality to the serial port.
#define CMDLINE_MAX_ARGS 1

//*****************************************************************************
// Declaration for the callback functions that will implement the command line
// functionality.  These functions get called by the command line interpreter
// when the corresponding command is typed into the command line.
extern int CMD_help(int argc, char **argv);
extern int CMD_set_on(int argc, char **argv);
extern int CMD_set_off(int argc, char **argv);

#endif //__XBEE_COMMANDS_H__
