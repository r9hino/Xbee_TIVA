/*
 * xbee_cmd_handler.h - Prototypes for command line processing functions.
 *
 *  Created on: 05-11-2014
 *      Author: Philippe Ilharreguy
 */

#ifndef XBEE_CMDLINE_H_
#define XBEE_CMDLINE_H_

//*****************************************************************************
// If building with a C++ compiler, make all of the definitions in this header
// have a C binding.
#ifdef __cplusplus
extern "C"
{
#endif

//*****************************************************************************
// Defines the value that is returned if the command is not found.
#define CMDLINE_BAD_CMD         (-1)

//*****************************************************************************
// Defines the value that is returned if there are too many arguments.
#define CMDLINE_TOO_MANY_ARGS   (-2)

//*****************************************************************************
// Defines the value that is returned if there are too few arguments.
#define CMDLINE_TOO_FEW_ARGS   (-3)

//*****************************************************************************
// Defines the value that is returned if an argument is invalid.
#define CMDLINE_INVALID_ARG   (-4)

//*****************************************************************************
// Command line function callback type.
typedef int8_t (*pfnCmdLine)(uint8_t argc, uint8_t *argv[]);

//*****************************************************************************
// Structure for an entry in the command list table.
typedef struct
{
    // A pointer to a string containing the name of the command.
    const uint8_t *pcCmd;

    // A function pointer to the implementation of the command.
    pfnCmdLine pfnCmd;

    // A pointer to a string of brief help text for the command.
    const uint8_t *pcHelp;
}
tCmdLineEntry;

//*****************************************************************************
// This is the command table that must be provided by the application.  The
// last element of the array must be a structure whose pcCmd field contains
// a NULL pointer.
extern tCmdLineEntry g_psCmdTable[];

//*****************************************************************************
// Prototypes for the APIs. Pass a string command as argument.
extern int8_t xbeeCmdLineProcess(uint8_t *pcCmdLine);

//*****************************************************************************
// Mark the end of the C bindings section for C++ compilers.
#ifdef __cplusplus
}
#endif

#endif /* XBEE_CMDLINE_H_ */
