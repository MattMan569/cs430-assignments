//
// HALshutdownAndRestart.h
//
// Copyright (c) 2019 Robert J. Hilderman.
// All Rights Reserved.
//

#include <iostream>
#include <string>
#include <stdlib.h>
#include <sys/shm.h>
#include <unistd.h>

using namespace std;

#ifndef HAL_PROCESS_DESCRIPTOR_H
    #include "HALprocessDescriptor.h"
#endif

extern int segmentID;
extern char *theClockBase;

extern void SendMessageToHAL9000 (processDescriptor process);
extern void SendMessageToHALkeyboardDriver (string pid, string systemCall);
extern void SendMessageToHALdisplayDriver (string pid, string systemCall, string buffer);
extern void SendMessageToHALdiskDriver (string pid, string systemCall, string fileName, string mode, string markerPosition, string buffer);

extern processDescriptor NullProcess ();

void ShutdownAndRestart (string command, string arguments []);

extern string itos (int i);

