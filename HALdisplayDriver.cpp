//
// HALdisplayDriver.cpp
//
// Copyright (c) 2019 Robert J. Hilderman.
// All Rights Reserved.
//

#include "HALdisplayDriver.h"

// The "Display Driver"

void HALdisplayDriver ()
{
    do
    {
        BlockSignals ("HALdisplayDriver");
        if (messageFromHALos)
        {
            messageFromHALos = 0;
            GetMessageFromHALos ();
            ProcessIORequest ();
            SendMessageToHALos ();
        }
        UnblockSignals ("HALdisplayDriver");
    } while (1);

    return;
}

void ProcessIORequest ()
{
    if (systemCall == "WRITE")
    {
        cout << buffer << flush;
        result = "DISPLAY_WRITE_OK";
    }
    else if (systemCall == "NEWLINE")
    {
        cout << endl;
        result = "DISPLAY_NEWLINE_OK";
    }
    else if (systemCall == "SHUTDOWN")
    {
        system ("HALdisplayDriverCleanup");
        exit (0);
    }

    return;
}

// The "Communication Media"

void GetMessageFromHALos ()
{
    ifstream ioRequestFile;

    ioRequestFile.open ("HALosToHALdisplayDriver");
    if (!ioRequestFile)
    {
        cout << "HALdisplayDriver: unable to initialize io request buffer" << endl;
        exit (1);
    }

    getline (ioRequestFile, pid);
    getline (ioRequestFile, systemCall);
    getline (ioRequestFile, buffer);

    #ifdef HALdisplayDriver_MESSAGE_TRACE_ON
    {
        cout << endl;
        cout << "HALdisplayDriver: message received from HALos for pid = " << pid << endl;
        cout << "    pid = " << pid << endl;
        cout << "    systemCall = " << systemCall << endl;
        cout << "    buffer = " << buffer << endl;
    }
    #endif

    if (!ioRequestFile)
    {
        cout << "HALdisplayDriver: message from HALos corrupted" << endl;
        exit (1);
    }

    ioRequestFile.close ();

    return;
}

void SendMessageToHALos ()
{
    ofstream ioResponseFile;
    union sigval dummyValue;

    ioResponseFile.open ("HALdisplayDriverToHALos");
    if (!ioResponseFile)
    {
        cout << "HALdisplayDriver: unable to initialize io response buffer" << endl;
        exit (1);
    }

    ioResponseFile << "INTERRUPT" << endl;
    ioResponseFile << systemCall << endl;
    ioResponseFile << pid << endl;
    ioResponseFile << result << endl;

    #ifdef HALdisplayDriver_MESSAGE_TRACE_ON
    {
        cout << endl;
        cout << "HALdisplayDriver: message sent to HALos for pid = " << pid << endl;
        cout << "    type = INTERRUPT" << endl;
        cout << "    systemCall = " << systemCall << endl;
        cout << "    pid = " << pid << endl;
        cout << "    result = " << result << endl;
    }
    #endif

    ioResponseFile.close ();

    if (sigqueue (HALosPid, SIGRTMIN, dummyValue) == -1)
    {
        cout << "HALdisplayDriver: io response signal not sent to HALos" << endl;
        exit (1);
    }

    return;
}
