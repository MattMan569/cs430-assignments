//
// HALcull.h
//
// Copyright (c) 2019 Robert J. Hilderman.
// All Rights Reserved.
//

#include <iostream>
#include <fstream>
#include <string>
#include <stdlib.h>
#include <signal.h>

using namespace std;

#ifndef HAL_QUEUE_H
    #include "HALreadyQueue.h"
#endif

#ifndef HAL_QUEUE_H
    #include "HALqueue.h"
#endif

#ifndef HAL_PROCESS_DESCRIPTOR_H
    #include "HALprocessDescriptor.h"
#endif

extern int nextPid;
extern int QUANTUM_LENGTH;
extern cpuSchedulingPolicyCriteria *cpuSchedulingPolicies;

extern ReadyQueueType readyQueue;
extern QueueType displayQueue;
extern QueueType keyboardQueue;
extern QueueType diskQueue;

struct message
{
    string type;
    string parameter1;
    string parameter2;
    string parameter3;
    string parameter4;
    string parameter5;
    string parameter6;
    string parameter7;
};

extern struct message HAL9000Message;
extern struct message HALkeyboardDriverMessage;
extern struct message HALdisplayDriverMessage;
extern struct message HALdiskDriverMessage;

extern processDescriptor cpuProcess;
extern processDescriptor keyboardProcess;
extern processDescriptor displayProcess;
extern processDescriptor diskProcess;

extern void HandleHAL9000Interrupt ();
extern void HandleSystemCall ();
extern void SetSystemCallParameters (processDescriptor &process, string systemCall, string systemCallParameter1, string systemCallParameter2, string systemCallParameter3, string systemCallBuffer, string systemCallresult);
extern void RunCpuScheduler ();
extern void ClearMessageParameters (message &HALMessage);
extern processDescriptor NullProcess ();
extern int UpdatePartitionTable (string runningPid, string &status);

extern void SendMessageToHAL9000 (processDescriptor process);
extern void SendReturnStatusToHALshell (string pid, string returnValue, string message);
extern void SendMessageToHALkeyboardDriver (string pid, string systemCall);
extern void SendMessageToHALdisplayDriver (string pid, string systemCall, string buffer);
extern void SendMessageToHALdiskDriver (string pid, string systemCall, string fileName, string mode, string markerPosition, string buffer);
extern void GetMessageFromHAL9000 ();
extern void GetMessageFromHALkeyboardDriver ();
extern void GetMessageFromHALdisplayDriver ();
extern void GetMessageFromHALdiskDriver ();
extern int RandomQuantumLengthAdjustment (int quantumLength);
extern string itos (int i);

void Cull (string command, string arguments []);
void SearchAndDestroy (string targetPid);
void PauseAnyExecutingProcess (string command);
void ConsumeUndeliveredMessages ();
void WaitForIoRequestsToFinish ();
void ExpungeProcess (string type, string targetPid);
void RestartCpuProcess ();
void RestartDeviceQueues ();
void CheckInDeviceQueues (string type, string targetPid);
bool ProcessInDeviceQueue (QueueType &queue, string type, string &targetPid);

extern volatile sig_atomic_t messageFromHAL9000;
extern volatile sig_atomic_t messageFromHALkeyboardDriver;
extern volatile sig_atomic_t messageFromHALdisplayDriver;
extern volatile sig_atomic_t messageFromHALdiskDriver;
extern sigset_t interruptMask;

extern void BlockSignals (string source);
extern void UnblockSignals (string source);
