//
// HALos.h
//
// Copyright (c) 2019 Robert J. Hilderman.
// All rights reserved.
//

#include <ctime>
#include <iostream>
#include <fstream>
#include <iomanip>
#include <sstream>
#include <stdlib.h>
#include <unistd.h>
#include <signal.h>
#include <limits.h>
#include <fcntl.h>
#include <sys/shm.h>
#include <sys/stat.h>

using namespace std;

#ifndef HAL_OS_INIT_H
#include "HALosInit.h"
#endif

#ifndef HAL_QUEUE_H
#include "HALreadyQueue.h"
#endif

#ifndef HAL_QUEUE_H
#include "HALqueue.h"
#endif

#ifndef HAL_MEMORY_H
#include "HALmemory.h"
#endif

#ifndef HAL_PARTITION_TABLE_H
#include "HALpartitionTable.h"
#endif

#ifndef HAL_GLOBALS_H
#include "HALglobals.h"
#endif

#ifndef HAL_SIGNALS_H
#include "HALsignals.h"
#endif

// Global Variables

time_t startTime;
time_t endTime;

ReadyQueueType readyQueue(READY_QUEUE_SIZE, NO_OF_READY_QUEUES, cpuSchedulingPolicies);
QueueType displayQueue(IO_QUEUE_SIZE);
QueueType keyboardQueue(IO_QUEUE_SIZE);
QueueType diskQueue(IO_QUEUE_SIZE);
MemoryType ram(PARTITION_SIZE, 1);
PartitionTableType partitionTable(NO_OF_PARTITIONS);

extern char **environ;

pid_t HAL9000Pid;
pid_t HALshellPid;
pid_t HALkeyboardDriverPid;
pid_t HALdisplayDriverPid;
pid_t HALdiskDriverPid;

int nextPid = 1;
bool cpuScheduler = true;

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

message HAL9000Message;
message HALkeyboardDriverMessage;
message HALdisplayDriverMessage;
message HALdiskDriverMessage;

processDescriptor cpuProcess;
processDescriptor keyboardProcess;
processDescriptor displayProcess;
processDescriptor diskProcess;

int segmentID;
size_t segmentSize;

char *theClock;
char *theClockBase;

// The "Operating System"

void HALos();
void HandleCommand(string command, string arguments[], string type);
void HandleNewProcess(string command, string arguments[], string type);
bool ProcessImageToFile(string pid, string parentPid, string command);
string InitializePidInProcessImage(string pid, string parentPid, string programLine,
                                   size_t &foundPid, size_t &foundParentPid, size_t &foundRunningTime);
void HandleHAL9000Interrupt();
void HandleSystemCall();
void HandleHALkeyboardDriverInterrupt();
void HandleHALdisplayDriverInterrupt();
void HandleHALdiskDriverInterrupt();
void SetSystemCallParameters(processDescriptor &process, string systemCall, string systemCallParameter1, string systemCallParameter2, string sytemcallParameter3, string systemCallBuffer, string systemCallResult);
void RunCpuScheduler();
void StopCpuScheduler();
void StartCpuScheduler();
// Assignment 4:
processDescriptor CloneProcessImage(processDescriptor parentProcess);
bool ChildProcessImageToFile(string pid, string parentPid);

// The "Communication Media"

string GetMessageFromHALshell(string arguments[], string &type);
void SendReturnStatusToHALshell(string pid, string returnValue, string message);
void SaveReturnStatusForHALshell(string pid, string returnValue, string message);
void GetMessageFromHAL9000();
void SendMessageToHAL9000(processDescriptor process);
void GetMessageFromHALkeyboardDriver();
void SendMessageToHALkeyboardDriver(string pid, string systemCall);
void GetMessageFromHALdisplayDriver();
void SendMessageToHALdisplayDriver(string pid, string systemCall, string buffer);
void GetMessageFromHALdiskDriver();
void SendMessageToHALdiskDriver(string pid, string systemCall, string fileName, string mode, string markerPosition, string buffer);

// Miscellaneous Functions and Procedures

void ClearMessageParameters(message &HALMessage);
int UpdatePartitionTable(string runningPid, string &action);
processDescriptor NullProcess();
unsigned int TimeSeed();
int GetRandomNoFromZeroTo(int maxNo);
int RandomQuantumLengthAdjustment(int baseQuantumLength);

// Numeric to String Function

string itos(int i);

/*

  #####
 #     #
#       # THE CODE THAT FOLLOWS SHOULD NOT BE CHANGED IN ANY WAY!
# STOP! #
#       # IF YOUR O/S IS NOT WORKING, YOU WON'T FIND THE PROBLEM HERE!
 #     #
  #####

*/

void Initialize();
int GetClockTicks();

extern void Compile(string arguments[]);
extern void Cull(string command, string arguments[]);
extern void ShutdownAndRestart(string command, string arguments[]);

volatile sig_atomic_t messageFromHALshell = 0;
volatile sig_atomic_t messageFromHAL9000 = 0;
volatile sig_atomic_t messageFromHALkeyboardDriver = 0;
volatile sig_atomic_t messageFromHALdisplayDriver = 0;
volatile sig_atomic_t messageFromHALdiskDriver = 0;

int main(int argc, char *argv[])
{
    segmentID = atoi(argv[1]);
    segmentSize = (size_t)atoi(argv[2]);

    Initialize();
    HALos();

    return 0;
}

void Initialize()
{
    cout << "HALbios: HALos OK" << endl;
    usleep(SLEEP_DELAY);

    cout << "HALos: initializing ..." << endl;
    usleep(SLEEP_DELAY);

    if ((theClockBase = (char *)shmat(segmentID, NULL, 0)) == (char *)-1)
    {
        cout << "HALos: unable to read clock" << endl;
        exit(1);
    }

    if ((sigemptyset(&interruptMask) == -1) ||
        (sigaddset(&interruptMask, SIGRTMIN) == -1))
    {
        cout << "HALos: unable to initialize signal mask" << endl;
        exit(1);
    }
    act.sa_sigaction = &SignalHandler;
    act.sa_mask = interruptMask;
    act.sa_flags = SA_SIGINFO;
    if ((sigaction(SIGRTMIN, &act, NULL) == -1))
    {
        cout << "HALos: unable to connect to HALshell" << endl;
        exit(1);
    }
    signal(SIGINT, SIG_IGN);
    usleep(SLEEP_DELAY);

    HAL9000Pid = getppid();

    cout << "HALos: loading HALkeyboardDriver ..." << endl;
    usleep(SLEEP_DELAY);
    HALkeyboardDriverPid = fork();
    if (HALkeyboardDriverPid < 0)
    {
        cout << "HALos: HALkeyboardDriver process creation failed" << endl;
        exit(1);
    }
    else if (HALkeyboardDriverPid == 0)
    {
        execle("HALkeyboardDriver", "HALkeyboardDriver", (char *)NULL, environ);
        cout << "HALos: HALkeyboardDriver failed to load" << endl;
        exit(1);
    }
    cout << "HALos: HALkeyboardDriver OK" << endl;
    usleep(SLEEP_DELAY);

    cout << "HALos: loading HALdisplayDriver ..." << endl;
    usleep(SLEEP_DELAY);
    HALdisplayDriverPid = fork();
    if (HALdisplayDriverPid < 0)
    {
        cout << "HALos: HALdisplayDriver process creation failed" << endl;
        exit(1);
    }
    else if (HALdisplayDriverPid == 0)
    {
        execle("HALdisplayDriver", "HALdisplayDriver", (char *)NULL, environ);
        cout << "HALos: HALdisplayDriver failed to load" << endl;
        exit(1);
    }
    cout << "HALos: HALdisplayDriver OK" << endl;
    usleep(SLEEP_DELAY);

    cout << "HALos: loading HALdiskDriver ..." << endl;
    usleep(SLEEP_DELAY);
    HALdiskDriverPid = fork();
    if (HALdiskDriverPid < 0)
    {
        cout << "HALos: HALdiskDriver process creation failed" << endl;
        exit(1);
    }
    else if (HALdiskDriverPid == 0)
    {
        execle("HALdiskDriver", "HALdiskDriver", (char *)NULL, environ);
        cout << "HALos: HALdiskDriver failed to load" << endl;
        exit(1);
    }
    cout << "HALos: HALdiskDriver OK" << endl;
    usleep(SLEEP_DELAY);

    cout << "HALos: loading HALshell ..." << endl;
    usleep(SLEEP_DELAY);
    HALshellPid = fork();
    if (HALshellPid < 0)
    {
        cout << "HALos: HALshell process creation failed" << endl;
        exit(1);
    }
    else if (HALshellPid == 0)
    {
        execle("HALshell", "HALshell", (char *)NULL, environ);
        cout << "HALos: HALshell failed to load" << endl;
        exit(1);
    }

    cpuProcess = NullProcess();
    keyboardProcess = NullProcess();
    displayProcess = NullProcess();
    diskProcess = NullProcess();

    srand(TimeSeed());

    return;
}

int GetClockTicks()
{
    int clockTicks;
    string clockTicksBuffer;
    int i;

    theClock = theClockBase;
    i = 0;
    while (*theClock != '\0')
    {
        clockTicksBuffer[i] = *theClock++;
        i++;
    }
    clockTicksBuffer[i] = '\0';
    clockTicks = atoi(clockTicksBuffer.c_str());

    return clockTicks;
}

static void SignalHandler(int signalNo, siginfo_t *info, void *context)
{
    if (signalNo == SIGRTMIN)
    {
        if (info->si_pid == HALshellPid)
        {
            messageFromHALshell = 1;
        }
        else if (info->si_pid == HAL9000Pid)
        {
            messageFromHAL9000 = 1;
        }
        else if (info->si_pid == HALkeyboardDriverPid)
        {
            messageFromHALkeyboardDriver = 1;
        }
        else if (info->si_pid == HALdisplayDriverPid)
        {
            messageFromHALdisplayDriver = 1;
        }
        else if (info->si_pid == HALdiskDriverPid)
        {
            messageFromHALdiskDriver = 1;
        }
    }
}
