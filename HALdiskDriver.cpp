//
// HALdiskDriver.cpp
//
// Copyright (c) 2019 Robert J. Hilderman.
// All Rights Reserved.
//

#include "HALdiskDriver.h"

// The "Disk Driver"

void HALdiskDriver ()
{
    do
    {
        BlockSignals ("HALdiskDriver");
        if (messageFromHALos)
        {
            messageFromHALos = 0;
            GetMessageFromHALos ();
            ProcessIORequest ();
            SendMessageToHALos ();
        }
        UnblockSignals ("HALdiskDriver");
    } while (1);

    return;
}

void ProcessIORequest ()
{
    fstream inOutFile;
    struct stat statusBuffer;
    size_t eofMarker;

    if (systemCall == "OPEN")
    {
        if (mode == "input")
        {
            // check for existence of file
            if (stat (fileName.c_str (), &statusBuffer) != -1)
            {
                result = "FILE_OPEN_OK";
            }
            else
            {
                result = "FILE_OPEN_FAILED";
            }
        }
        else // (mode == "output")
        {
            // create empty file
            inOutFile.open (fileName.c_str (), fstream::trunc | fstream::out);
            if (inOutFile)
            {
                result = "FILE_OPEN_OK";
            }
            else
            {
                result = "FILE_OPEN_FAILED";
            }
            inOutFile.close ();
        }
    }
    else if (systemCall == "READ")
    {
        inOutFile.open (fileName.c_str (), fstream::ate | fstream::in);
        if (inOutFile)
        {
            eofMarker = inOutFile.tellg ();
            if (atoi (markerPosition.c_str ()) != eofMarker)
            {
                inOutFile.seekg (atoi (markerPosition.c_str ()));
                getline (inOutFile, buffer);
                if (inOutFile)
                {
                    markerPosition = itos (inOutFile.tellg ());
                    result = "FILE_READ_OK";
                }
                else
                {
                    buffer = "";
                    markerPosition = "";
                    result = "FILE_READ_FAILED";
                }
            }
            else
            {
                buffer = "";
                markerPosition = "EOF";
                result = "FILE_AT_END";
            }
        }
        else
        {
            buffer = "";
            markerPosition = "";
            result = "FILE_READ_FAILED";
        }
        inOutFile.close ();
    }
    else if (systemCall == "WRITE")
    {
        inOutFile.open (fileName.c_str (), fstream::app | fstream ::out);
        if (inOutFile)
        {
            inOutFile << buffer << flush;
            markerPosition = "";
            result = "FILE_WRITE_OK";
        }
        else
        {
            markerPosition = "";
            result = "FILE_WRITE_FAILED";
        }
        inOutFile.close ();
    }
    else if (systemCall == "NEWLINE")
    {
        inOutFile.open (fileName.c_str (), fstream::app | fstream ::out);
        if (inOutFile)
        {
            inOutFile << endl;
            markerPosition = "";
            result = "FILE_NEWLINE_OK";
        }
        else
        {
            markerPosition = "";
            result = "FILE_NEWLINE_FAILED";
        }
        inOutFile.close ();
    }
    else if (systemCall == "SHUTDOWN")
    {
        system ("HALdiskDriverCleanup");
        exit (0);
    }

    return;
}

// The "Communication Media"

void GetMessageFromHALos ()
{
    ifstream ioRequestFile;

    ioRequestFile.open ("HALosToHALdiskDriver");
    if (!ioRequestFile)
    {
        cout << "HALdiskDriver: unable to open io request buffer" << endl;
        exit (1);
    }

    getline (ioRequestFile, pid);
    getline (ioRequestFile, systemCall);
    getline (ioRequestFile, fileName);
    getline (ioRequestFile, mode);
    getline (ioRequestFile, markerPosition);
    getline (ioRequestFile, buffer);

    #ifdef HALdiskDriver_MESSAGE_TRACE_ON
    {
        cout << endl;
        cout << "HALdiskDriver: message received from HALos for pid = " << pid << endl;
        cout << "    pid = " << pid << endl;
        cout << "    systemCall = " << systemCall << endl;
        cout << "    fileName = " << fileName << endl;
        cout << "    mode = " << mode << endl;
        cout << "    markerPosition = " << markerPosition << endl;
        cout << "    buffer = " << buffer << endl;
    }
    #endif

    if (!ioRequestFile)
    {
        cout << "HALdiskDriver: message from HALos corrupted" << endl;
        exit (1);
    }

    ioRequestFile.close ();

    return;
}

void SendMessageToHALos ()
{
    static int seqNo = 0;
    static string fileNamePrefix = "HALdiskDriverToHALos_";
    string fileName;
    ofstream ioResponseFile;
    union sigval dummyValue;

    seqNo ++;
    fileName = fileNamePrefix + itos (seqNo);
    ioResponseFile.open (fileName.c_str ());
    if (!ioResponseFile)
    {
        cout << "HALdiskDriver: unable to open io response buffer" << endl;
        exit (1);
    }

    ioResponseFile << "INTERRUPT" << endl;
    ioResponseFile << systemCall << endl;
    ioResponseFile << pid << endl;
    ioResponseFile << markerPosition << endl;
    ioResponseFile << buffer << endl;
    ioResponseFile << result << endl;

    #ifdef HALdiskDriver_MESSAGE_TRACE_ON
    {
        cout << endl;
        cout << "HALdiskDriver: message sent to HALos for pid = " << pid << endl;
        cout << "    type = INTERRUPT" << endl;
        cout << "    systemCall = " << systemCall << endl;
        cout << "    pid = " << pid << endl;
        cout << "    markerPosition = " << markerPosition << endl;
        cout << "    buffer = " << buffer << endl;
        cout << "    result = " << result << endl;
    }
    #endif

    ioResponseFile.close ();

    if (sigqueue (HALosPid, SIGRTMIN, dummyValue) == -1)
    {
        cout << "HALdiskDriver: error sending io response signal to HALos" << endl;
        exit (1);
    }

    if (seqNo == INT_MAX)
    {
        seqNo = 1;
    }

    return;
}

string itos (int i)
{
    stringstream s;

    s << i;

    return s.str ();
}
