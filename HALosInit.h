//
// HALosInit.h
//
// Copyright (c) 2019 Robert J. Hilderman.
// All Rights Reserved.
//

#define HAL_OS_INIT_H

#include <iostream>
#include <fstream>

using namespace std;

#ifndef HAL_PROCESS_DESCRIPTOR_H
    #include "HALprocessDescriptor.h"
#endif

#ifndef HAL_CPU_SCHEDULING_POLICY_CRITERIA_H
    #include "HALcpuSchedulingPolicyCriteria.h"
#endif

int READY_QUEUE_SIZE;
int IO_QUEUE_SIZE;
int QUANTUM_LENGTH;
int NO_OF_READY_QUEUES;
cpuSchedulingPolicyCriteria *cpuSchedulingPolicies;
int PARTITION_SIZE;
int NO_OF_PARTITIONS;

int GetHALosVariables ()
{
    int i;

    ifstream osFile;
    string variableDescription;
    string variableValue;

    osFile.open ("HALosVariables");
    if (!osFile)
    {
        cout << "HALos: unable to read os variables file" << endl;
        exit (1);
    }

    osFile >> variableDescription;
    osFile.ignore (256, ':');
    getline (osFile, variableValue);
    READY_QUEUE_SIZE = atoi (variableValue.c_str ());

    osFile >> variableDescription;
    osFile.ignore (256, ':');
    getline (osFile, variableValue);
    IO_QUEUE_SIZE = atoi (variableValue.c_str ());

    osFile >> variableDescription;
    osFile.ignore (256, ':');
    getline (osFile, variableValue);
    QUANTUM_LENGTH = atoi (variableValue.c_str ());

    osFile >> variableDescription;
    osFile.ignore (256, ':');
    getline (osFile, variableValue);
    NO_OF_READY_QUEUES = atoi (variableValue.c_str ());

    cpuSchedulingPolicies = new cpuSchedulingPolicyCriteria [NO_OF_READY_QUEUES];

    for (i = 0; i < NO_OF_READY_QUEUES; i ++)
    {
        osFile >> variableDescription;
        osFile.ignore (256, ':');
        getline (osFile, variableValue);
        cpuSchedulingPolicies [i].cpuSchedulingPolicy = variableValue;

        osFile >> variableDescription;
        osFile.ignore (256, ':');
        getline (osFile, variableValue);
        cpuSchedulingPolicies [i].quantumLengthMultiplier = atoi (variableValue.c_str ());

        osFile >> variableDescription;
        osFile.ignore (256, ':');
        getline (osFile, variableValue);
        cpuSchedulingPolicies [i].interruptsUntilMoveDown = atoi (variableValue.c_str ());

        osFile >> variableDescription;
        osFile.ignore (256, ':');
        getline (osFile, variableValue);
        cpuSchedulingPolicies [i].interruptsUntilMoveUp = atoi (variableValue.c_str ());
    }

    osFile.close ();

    return 0;
}

int GetHALbiosMemorySizeVariable ()
{
    ifstream biosFile;
    string uselessString;
    string variableValue;

    biosFile.open ("HALbiosVariables");
    if (!biosFile)
    {
        cout << "HALos: unable to determine memory size" << endl;
        exit (1);
    }

    biosFile >> uselessString;
    biosFile.ignore (256, ':');
    getline (biosFile, variableValue);
    PARTITION_SIZE = atoi (variableValue.c_str ());

    biosFile >> uselessString;
    biosFile.ignore (256, ':');
    getline (biosFile, variableValue);
    NO_OF_PARTITIONS = atoi (variableValue.c_str ());

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

Dummy A (GetHALosVariables ());
Dummy B (GetHALbiosMemorySizeVariable ());
