//
// HAL9000Init.h
//
// Copyright (c) 2019 Robert J. Hilderman.
// All Rights Reserved.
//

#define HAL_9000_INIT_H

#include <iostream>
#include <fstream>
#include <sys/shm.h>
#include <string.h>

using namespace std;

#define SEGMENT_KEY ((key_t) 970303)

int segmentID;
int segmentPermissions = SHM_R | SHM_W | IPC_CREAT;
size_t segmentSize = 10;

char *theClock;
char *theClockBase;

const int MAX_BIOS_VARIABLES = 10;

int PARTITION_SIZE;
int NO_OF_PARTITIONS;
string HALos;

memoryCell biosVariables [MAX_BIOS_VARIABLES];

int VerifyHost ()
{
    char hostName [MAX_LENGTH];

    gethostname (hostName, sizeof (hostName));
    if (strcmp (hostName, "hercules") == 0)
    {
        cout << "HALstart: HAL9000 virtual machine will not run on hercules" << endl;
        exit (1);
    }
    else if (strcmp (hostName, "a049831") == 0)
    {
        cout << "HALstart: HAL9000 virtual machine will not run on a049831" << endl;
        exit (1);
    }

    return 0;
}

int CreateClock ()
{
    cout << "HALstart: HAL9000 OK" << endl;
    usleep (SLEEP_DELAY);

    cout << "HAL9000: initializing ..." << endl;
    usleep (SLEEP_DELAY);

    cout << "HAL9000: boot sequence started ..." << endl;
    usleep (SLEEP_DELAY);

    cout << "HAL9000: checking clock ..." << endl;
    usleep (SLEEP_DELAY);

    if ((segmentID = shmget (SEGMENT_KEY, segmentSize, segmentPermissions)) == -1)
    {
        cout << "HAL9000: clock failure" << endl;
        exit (1);
    }

    if ((theClockBase = (char *) shmat (segmentID, NULL, 0)) == (char *) -1)
    {
        cout << "HAL9000: clock failure" << endl;
        exit (1);
    }

    theClock = theClockBase;
    *theClock ++ = '0';
    *theClock ++ = '\0';

    cout << "HAL9000: clock OK" << endl;

    return 0;
}

int GetHALbiosVariables ()
{
    ifstream biosFile;
    int i;
    string variableDescription;
    string variableValue;

    cout << "HAL9000: loading HALbios ..." << endl;

    for (i = 0; i < MAX_BIOS_VARIABLES; i ++)
    {
        biosVariables [i].symbol = "";
        biosVariables [i].value = "";
    }

    biosFile.open ("HALbiosVariables");
    if (!biosFile)
    {
        cout << "HAL9000: unable to read bios variables file" << endl;
        exit (1);
    }

    biosFile >> variableDescription;
    biosFile.ignore (256, ':');
    getline (biosFile, variableValue);
    PARTITION_SIZE = atoi (variableValue.c_str ());
    biosVariables [0].symbol = variableDescription;
    biosVariables [0].value = variableValue;

    biosFile >> variableDescription;
    biosFile.ignore (256, ':');
    getline (biosFile, variableValue);
    NO_OF_PARTITIONS = atoi (variableValue.c_str ());
    biosVariables [1].symbol = variableDescription;
    biosVariables [1].value = variableValue;

    biosFile >> variableDescription;
    biosFile.ignore (256, ':');
    getline (biosFile, variableValue);
    HALos = variableValue;
    biosVariables [2].symbol = variableDescription;
    biosVariables [2].value = variableValue;

    biosFile.close ();
    usleep (SLEEP_DELAY);

    cout << "HAL9000: HALbios OK" << endl;
    usleep (SLEEP_DELAY);

    return 0;
}

class Dummy
{
public:
    Dummy (int dummy);
private:
    int DUMMY;
};

Dummy::Dummy (int dummy)
{
    DUMMY = dummy;
}

Dummy A (VerifyHost ());
Dummy B (CreateClock ());
Dummy C (GetHALbiosVariables ());
