//
// HALprocessDescriptor.h
//
// Copyright (c) 2019 Robert J. Hilderman.
// All rights reserved.
//

#define HAL_PROCESS_DESCRIPTOR_H

#include <string>

using namespace std;

#ifndef HAL_FILE_TABLE_H
    #include "HALfileTable.h"
#endif

#ifndef HAL_GLOBALS_H
    #include "HALglobals.h"
#endif

struct processDescriptor
{
    processDescriptor () : fileTable () {}
    string pid;
    string type;
    string action;
    string command;
    string arguments [MAX_COMMAND_LINE_ARGUMENTS];
    string returnValue;
    int queueNo;
    int quantumLength;
    int partitionNo;
    string direction;
    int interruptCounter;
    int timerInterruptCounter;
    int systemCallInterruptCounter;
    int priority;
    int creationTime;
    int runningTime;
    FileTableType fileTable;
    string systemCall;
    string systemCallParameter1;
    string systemCallParameter2;
    string systemCallParameter3;
    string systemCallBuffer;
    string systemCallResult;
};
