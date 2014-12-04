/*
 * xbee_cmd_handler.c - Parse and process command received from xbee data messages.
 *
 *  Created on: 05-11-2014
 *      Author: Philippe Ilharreguy
 */

#include <stdint.h>
#include <stdbool.h>
#include "lib_utils/ustdlib.h"
#include "lib_xbee/xbee_data_parser.h"

//*****************************************************************************
// Defines the maximum number of arguments that can be parsed.
#ifndef CMDLINE_MAX_ARGS
#define CMDLINE_MAX_ARGS        2
#endif

//*****************************************************************************
// An array to hold the pointers to the command line arguments.
static uint8_t *g_ppcArgv[CMDLINE_MAX_ARGS + 1];

//*****************************************************************************
// Process a command line string into arguments and execute the command.
//
// \param pcCmdLine points to a string that contains a command line that was
// obtained by an application by some means.
//
// This function will take the supplied command line string and break it up
// into individual arguments.  The first argument is treated as a command and
// is searched for in the command table.  If the command is found, then the
// command function is called and all of the command line arguments are passed
// in the normal argc, argv form.
//
// The command table is contained in an array named <tt>g_psCmdTable</tt>
// containing <tt>tCmdLineEntry</tt> structures which must be provided by the
// application.  The array must be terminated with an entry whose \b pcCmd
// field contains a NULL pointer.
//
// \return Returns \b CMDLINE_BAD_CMD if the command is not found,
// \b CMDLINE_TOO_MANY_ARGS if there are more arguments than can be parsed.
// Otherwise it returns the code that was returned by the command function.

int8_t xbeeCmdLineProcess(uint8_t *xbeeCmdLine) {
    uint8_t *xbeeChar;
    uint8_t ui8Argc;
    bool bFindArg = true;
    tCmdLineEntry *psCmdEntry;

    // Initialize the argument counter, and point to the beginning of the command line string.
    ui8Argc = 0;
    xbeeChar = xbeeCmdLine;

    // Advance through the command line until a zero character is found.
    while (*xbeeChar != 0)     {
        // If there is a space, then replace it with a zero, and set the flag to search for the next argument.
        if (*xbeeChar == ' ') {
            *xbeeChar = 0;
            bFindArg = true;
        }

        // Otherwise it is not a space, so it must be a character that is part of the cmd or an argument.
        else {
            // If bFindArg is set, then that means we are looking for the start of the cmd or next argument.
            if (bFindArg) {
                // As long as the maximum number of arguments has not been
                // reached, then save the pointer to the start of this new arg
                // in the argv array, and increment the count of args, argc.
                if (ui8Argc < CMDLINE_MAX_ARGS) {
                    g_ppcArgv[ui8Argc] = xbeeChar;
                    ui8Argc++;
                    bFindArg = false;
                }

                // The maximum number of arguments has been reached so return the error.
                else {
                    return(CMDLINE_TOO_MANY_ARGS);
                }
            }
        }

        // Advance to the next character in the command line.
        xbeeChar++;
    }

    // If one or more arguments was found, then process the command.
    if (ui8Argc) {
        // Start at the beginning of the command table, to look for a matching command.
        psCmdEntry = &g_psCmdTable[0];

        // Search through the command table until a null command string is
        // found, which marks the end of the table.
        while (psCmdEntry->pcCmd) {
            // If this command entry command string matches argv[0], then call
            // the function for this command, passing the command line arguments.
            if (!ustrcmp((char *)g_ppcArgv[0], (char *)psCmdEntry->pcCmd)) {
                return(psCmdEntry->pfnCmd(ui8Argc, g_ppcArgv));
            }

            // Not found, so advance to the next entry.
            psCmdEntry++;
        }
    }

    // Fall through to here means that no matching command was found, so return an error.
    return(CMDLINE_BAD_CMD);
}