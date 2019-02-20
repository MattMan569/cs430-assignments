//
// HALshutdownAndRestart.cpp
//
// Copyright (c) 2019 Robert J. Hilderman.
// All Rights Reserved.
//

#include "HALshutdownAndRestart.h"

void ShutdownAndRestart (string command, string arguments [])
{
    int i;
    processDescriptor process;

    process = NullProcess ();

    cout << "HALos: terminating ..." << endl;
    system ("HALosCleanup");
    usleep (SLEEP_DELAY);

    process.pid = itos (0);
    process.action = "SHUTDOWN_OR_RESTART";
    process.command = command;
    for (i = 0; i < MAX_COMMAND_LINE_ARGUMENTS; i ++)
    {
        process.arguments [i] = arguments [i];
    }
    process.partitionNo = 0;
    SendMessageToHAL9000 (process);
    SendMessageToHALkeyboardDriver ("0", "SHUTDOWN");
    SendMessageToHALdisplayDriver ("0", "SHUTDOWN", "");
    SendMessageToHALdiskDriver ("0", "SHUTDOWN", "", "", "", "");

    if (shmdt (theClockBase) == -1)
    {
        cout << "HALos: clock failure" << endl;
    }

    exit (0);
}
