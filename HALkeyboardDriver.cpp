//
// HALkeyboardDriver.cpp
//
// Copyright (c) 2019 Robert J. Hilderman.
// All Rights Reserved.
//

#include "HALkeyboardDriver.h"

// The "Keyboard Driver"

void HALkeyboardDriver ()
{
    do
    {
        BlockSignals ("HALkeyboardDriver");
        if (messageFromHALos)
        {
            messageFromHALos = 0;
            GetMessageFromHALos ();
            ProcessIORequest ();
            if (result == "KEYBOARD_READ_OK")
            {
                SendMessageToHALos ();
            }
        }
        UnblockSignals ("HALkeyboardDriver");
    } while (1);

    return;
}

void ProcessIORequest ()
{
    cullProcess = 0;

    if (systemCall == "READ")
    {
        getline (cin, buffer);
        if (cullProcess == 1)
        {
            cin.clear ();
            result = "PROCESS_CULLED";
        }
        else
        {
            result = "KEYBOARD_READ_OK";
        }
    }
    else if (systemCall == "SHUTDOWN")
    {
        system ("HALkeyboardDriverCleanup");
        exit (0);
    }

    return;
}

// The "Communication Media"

void GetMessageFromHALos ()
{
    ifstream ioRequestFile;

    ioRequestFile.open ("HALosToHALkeyboardDriver");
    if (!ioRequestFile)
    {
        cout << "HALkeyboardDriver: unable to initialize io request buffer" << endl;
        exit (1);
    }

    getline (ioRequestFile, pid);
    getline (ioRequestFile, systemCall);

    #ifdef HALkeyboardDriver_MESSAGE_TRACE_ON
    {
        cout << endl;
        cout << "HALkeyboardDriver: message received from HALos for pid = " << pid << endl;
        cout << "    pid = " << pid << endl;
        cout << "    systemCall = " << systemCall << endl;
    }
    #endif

    if (!ioRequestFile)
    {
        cout << "HALkeyboardDriver: message from HALos corrupted" << endl;
        exit (1);
    }

    ioRequestFile.close ();

    return;
}

void SendMessageToHALos ()
{
    ofstream ioResponseFile;
    union sigval dummyValue;

    ioResponseFile.open ("HALkeyboardDriverToHALos");
    if (!ioResponseFile)
    {
        cout << "HALkeyboardDriver: unable to initialize io response buffer" << endl;
        exit (1);
    }

    ioResponseFile << "INTERRUPT" << endl;
    ioResponseFile << systemCall << endl;
    ioResponseFile << pid << endl;
    ioResponseFile << buffer << endl;
    ioResponseFile << result << endl;

    #ifdef HALkeyboardDriver_MESSAGE_TRACE_ON
    {
        cout << endl;
        cout << "HALkeyboardDriver: message sent to HALos for pid = " << pid << endl;
        cout << "    type = INTERRUPT" << endl;
        cout << "    systemCall = " << systemCall << endl;
        cout << "    pid = " << pid << endl;
        cout << "    buffer = " << buffer << endl;
        cout << "    result = " << result << endl;
    }
    #endif

    ioResponseFile.close ();

    if (sigqueue (HALosPid, SIGRTMIN, dummyValue) == -1)
    {
        cout << "HALkeyboardDriver: io response signal not sent to HALos" << endl;
        exit (1);
    }

    return;
}
