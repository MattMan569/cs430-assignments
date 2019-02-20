//
// HAL9000.h
//
// Copyright (c) 2019 Robert J. Hilderman.
// All Rights Reserved.
//

#include <iostream>
#include <fstream>
#include <iomanip>
#include <sstream>
#include <stdlib.h>
#include <unistd.h>
#include <signal.h>
#include <limits.h>
#include <sys/stat.h>

using namespace std;

#ifndef HAL_GLOBALS_H
    #include "HALglobals.h"
#endif

#ifndef HAL_MEMORY_H
    #include "HALmemory.h"
#endif

#ifndef HAL_9000_INIT_H
    #include "HAL9000Init.h"
#endif

#ifndef HAL_SIGNALS_H
    #include "HALsignals.h"
#endif

// Global Variables

MemoryType ram (PARTITION_SIZE, NO_OF_PARTITIONS);

extern char **environ;

pid_t HALstartPid;
pid_t HALosPid;
int partitionNo;

struct message
{
    string parameter1;
    string parameter3;
    string parameter4;
    string parameter5;
    string parameter6;
};

message HALosMessage;

// The "Processor"

void HAL9000 ();
void StartMain (string arguments []);
bool ExecuteOneInstruction (string buffer, string result, bool &systemCall);
memoryCell FetchInstruction ();
bool DecodeInstruction (memoryCell contents);
bool ExecuteInstruction (int currentProgramTextAddress, memoryCell contents, string restartInstructionStatusFlag, string buffer, string result);
void EndMain ();

// The "Communication Media"

string GetMessageFromHALos (int &pid, string &command, string arguments [], string &buffer, string &result, int &quantumLength);
void SendMessageToHALos (string type, string parameter1, string parameter2, string parameter3, string parameter4, string parameter5, string parameter6, string parameter7);

// "Built-in" Instructions

void Comment ();
void Call (int currentProgramTextAddress, memoryCell contents);
void Return (memoryCell contents);
void Push (memoryCell contents);
memoryCell DetermineMemoryCellContentsForReturnAndPushCommands (memoryCell contents1);
void Set (memoryCell contents);
void Compare (string dataType);
void CompareEndOfFile ();
void CompareDataTypes (string dataType);
void CompareValues ();
void Pop ();
void Jump (memoryCell contents);
void DoTheMath (string operation);
void Join ();
void CoreSnapShot ();

// System Calls

void Open (string symbol, string restartInstructionStatusFlag, string result);
void Read (string symbol, string restartInstructionStatusFlag, string buffer, string result);
void Write (string symbol, string restartInstructionStatusFlag, string result);
void Newline (string symbol, string restartInstructionStatusFlag, string result);
void Close (string symbol, string restartInstructionStatusFlag, string result);

// The "Backing Store"

void ProcessImageToMemory (int pid, int quantumLength);
void ProcessImageToFile (int pid, string location);
bool AddressField (string symbol, int i, int globalSymbolsTableStartAddress, int globalSymbolsTableEndAddress, int functionCallStackStartAddress, int functionCallStackEndAddress);

// Local and Global Variable Interfaces

void AllocateLocalSymbol (memoryCell contents);
void AssignTypeToLocalSymbol (memoryCell contents);
void AllocateLocalArray (memoryCell contents);
int GetLocalSymbolAddress (string value);
int GetGlobalSymbolAddress (string value);
string GetDataType (string symbol);
string GetDataTypeCategory (string symbol);
bool IsInteger (string value);
bool IsFloat (string value);

// Kernel Variable Interfaces

int GetKernelVariableIntegerValue (string kernelVariableDescription);
string GetKernelVariableStringValue (string kernelVariableDescription);
void SetKernelVariableValue (string kernelVariableDescription, string value);

// Numeric To String Functions

string itos (int i);
string dtos (double d);

/*

  #####
 #     #
#       # THE CODE THAT FOLLOWS SHOULD NOT BE CHANGED IN ANY WAY!
# STOP! #
#       # IF YOUR O/S IS NOT WORKING, YOU WON'T FIND THE PROBLEM HERE!
 #     #
  #####

*/

void Initialize ();
void SetClockTicks (int clockTicks);
int GetClockTicks ();
void InitializeMemory ();
void GetMemorySegmentParameters ();
void SetMemorySegmentBoundaries ();
void SetKernelVariables ();
void SetTableAndStackStartAddresses ();
int GetMemorySegmentBoundary (string segmentStartAddressDescription, int &segmentSize);
void CoreDump ();
void ShutdownOrRestart (string command);
void Shutdown ();
void Restart ();

volatile sig_atomic_t messageFromHALos = 0;

int main (int argc, char *argv [])
{
    Initialize ();
    HAL9000 ();

    return 0;
}

void Initialize ()
{
    cout << "HALbios: initializing ..." << endl;
    usleep (SLEEP_DELAY);

    cout << "HALbios: checking memory ..." << endl;
    InitializeMemory ();
    usleep (SLEEP_DELAY);

    cout << "HALbios: memory OK" << endl;
    usleep (SLEEP_DELAY);

    SetClockTicks (0);

    if ((sigemptyset (&interruptMask) == -1) ||
        (sigaddset (&interruptMask, SIGRTMIN) == -1))
    {
        cout << "HALbios: unable to initialize signal mask" << endl;
        CoreDump ();
        exit (1);
    }
    act.sa_sigaction = &SignalHandler;
    act.sa_mask = interruptMask;
    act.sa_flags = SA_SIGINFO;
    if ((sigemptyset (&act.sa_mask) == -1) ||
        (sigaction (SIGRTMIN, &act, NULL) == -1))
    {
        cout << "HALbios: problem loading HALos" << endl;
        CoreDump ();
        exit (1);
    }
    signal (SIGINT, SIG_IGN);
    usleep (SLEEP_DELAY);

    HALstartPid = getppid ();

    cout << "HALbios: loading HALos ..." << endl;
    usleep (SLEEP_DELAY);
    HALosPid = fork ();
    if (HALosPid < 0)
    {
        cout << "HALbios: HALos process creation failed" << endl;
        CoreDump ();
        exit (1);
    }
    else if (HALosPid == 0)
    {
        execle (HALos.c_str (), HALos.c_str (), (char *) (itos (segmentID)).c_str (),(char *) (itos ((int) segmentSize)).c_str (), (char *) NULL, environ);
        cout << "HALbios: HALos failed to load" << endl;
        CoreDump ();
        exit (1);
    }

    return;
}

void SetClockTicks (int clockTicks)
{
    string clockTicksBuffer;
    int i;

    clockTicksBuffer = itos (clockTicks);

    theClock = theClockBase;
    i = 0;
    while (clockTicksBuffer [i] != '\0')
    {
        *theClock ++ = clockTicksBuffer [i];
        i ++;
    }
    *theClock = '\0';

    return;
}

int GetClockTicks ()
{
    int clockTicks;
    string clockTicksBuffer;
    int i;

    theClock = theClockBase;
    i = 0;
    while (*theClock != '\0')
    {
        clockTicksBuffer [i] = *theClock ++;
        i ++;
    }
    clockTicksBuffer [i] = '\0';
    clockTicks = atoi (clockTicksBuffer.c_str ());

    return clockTicks;
}

void InitializeMemory ()
{
    for (partitionNo = 0; partitionNo < NO_OF_PARTITIONS; partitionNo ++)
    {
        GetMemorySegmentParameters ();
        SetMemorySegmentBoundaries ();
        SetKernelVariables ();
    }

    partitionNo = 0;
    ProcessImageToFile (0, "template");

    return;
}

void GetMemorySegmentParameters ()
{
    ifstream memorySegmentsFile;
    string symbol;
    string value;
    int i;

    memorySegmentsFile.open ("HALmemoryVariables");
    if (!memorySegmentsFile)
    {
        cout << "HAL9000: unable to read memory segments file" << endl;
        CoreDump ();
        exit (1);
    }

    ram.ResetP ();
    memorySegmentsFile >> symbol;
    while (memorySegmentsFile)
    {
        ram.IterateUp ();
        memorySegmentsFile.ignore (256, ':');
        getline (memorySegmentsFile, value);
        if (value == "0d_0")
        {
            value = "0";
        }
        else if (value == "0d_null")
        {
            value = "null";
        }
        ram.Write (symbol, value, partitionNo);
        memorySegmentsFile >> symbol;
    }
    memorySegmentsFile.close ();

    for (i = 0; i < MAX_BIOS_VARIABLES; i ++)
    {
        if (biosVariables [i].symbol.length () > 0)
        {
            ram.IterateUp ();
            ram.Write (biosVariables [i].symbol, biosVariables [i].value, partitionNo);
        }
    }

    return;
}

void SetMemorySegmentBoundaries ()
{
    int address;
    memoryCell contents;
    int i;

    address = 0;
    ram.ResetP ();
    ram.IterateUp ();
    ram.IterateUp ();

    for (i = 1; i <= 3; i ++)
    {
        contents = ram.Read (partitionNo);
        address = address + atoi (contents.value.c_str ());
        ram.IterateUp ();
        ram.ReWrite ("", itos (address), partitionNo);
        ram.IterateUp ();
    }

    address = PARTITION_SIZE;

    ram.IterateUp ();
    for (i = 1; i <= 2; i ++)
    {
        ram.ReWrite ("", itos (address - 1), partitionNo);
        ram.IterateUp ();
        contents = ram.Read (partitionNo);
        address = address - atoi (contents.value.c_str ());
        ram.IterateUp ();
    }

    return;
}

void SetKernelVariables ()
{
    ifstream kernelVariablesFile;
    int address;
    int segmentSize;
    string symbol;
    string value;

    kernelVariablesFile.open ("HALkernelVariables");
    if (!kernelVariablesFile)
    {
        cout << "HAL9000: unable to read kernel variables file" << endl;
        CoreDump ();
        exit (1);
    }

    address = GetMemorySegmentBoundary ("KERNEL_SPACE_START_ADDRESS", segmentSize);
    address = address + segmentSize;
    ram.SetP (address);
    kernelVariablesFile >> symbol;
    while (kernelVariablesFile)
    {
        ram.IterateDown ();
        kernelVariablesFile.ignore (256, ':');
        getline (kernelVariablesFile, value);
        if (value == "0d_null")
        {
            value = "null";
        }
        ram.Write (symbol, value, partitionNo);
        kernelVariablesFile >> symbol;
    }
    kernelVariablesFile.close ();
    SetTableAndStackStartAddresses ();

    return;
}

void SetTableAndStackStartAddresses ()
{
    int address;
    int segmentSize;
    int kernelVariableValue;

    address = GetMemorySegmentBoundary ("GLOBAL_SYMBOLS_TABLE_START_ADDRESS", segmentSize);
    kernelVariableValue = GetKernelVariableIntegerValue ("LAST_GLOBAL_SYMBOLS_TABLE_ADDRESS");
    SetKernelVariableValue ("LAST_GLOBAL_SYMBOLS_TABLE_ADDRESS", itos (address - 1));

    address = GetMemorySegmentBoundary ("GLOBAL_VALUES_TABLE_START_ADDRESS", segmentSize);
    kernelVariableValue = GetKernelVariableIntegerValue ("LAST_GLOBAL_VALUES_TABLE_ADDRESS");
    SetKernelVariableValue ("LAST_GLOBAL_VALUES_TABLE_ADDRESS", itos (address - 1));

    address = GetMemorySegmentBoundary ("FUNCTION_CALL_STACK_START_ADDRESS", segmentSize);
    kernelVariableValue = GetKernelVariableIntegerValue ("TOP_FUNCTION_CALL_STACK_ADDRESS");
    SetKernelVariableValue ("TOP_FUNCTION_CALL_STACK_ADDRESS", itos (address + 1));

    address = GetMemorySegmentBoundary ("FUNCTION_CALL_VALUES_STACK_START_ADDRESS", segmentSize);
    kernelVariableValue = GetKernelVariableIntegerValue ("TOP_FUNCTION_CALL_VALUES_STACK_ADDRESS");
    SetKernelVariableValue ("TOP_FUNCTION_CALL_VALUES_STACK_ADDRESS", itos (address + 1));

    return;
}

int GetMemorySegmentBoundary (string segmentStartAddressDescription, int &segmentSize)
{
    int startAddress;
    memoryCell contents;

    ram.ResetP ();
    ram.IterateUp ();
    while (1)
    {
        contents = ram.Read (partitionNo);
        if (contents.symbol == segmentStartAddressDescription)
        {
            break;
        }
        ram.IterateUp ();
    }
    startAddress = atoi (contents.value.c_str ());
    ram.IterateUp ();
    contents = ram.Read (partitionNo);
    segmentSize = atoi (contents.value.c_str ());

    return startAddress;
}

void CoreDump ()
{
    int pid;

    pid = GetKernelVariableIntegerValue ("PID");
    ProcessImageToFile (pid, "coredump");

    return;
}

void ShutdownOrRestart (string command)
{
    if (command == "shutdown")
    {
        Shutdown ();
    }
    else if (command == "restart")
    {
        Restart ();
    }

    return;
}

void Shutdown ()
{
    cout << "HAL9000: powering down ..." << endl;

    if (shmdt (theClockBase) == -1)
    {
        cout << "HAL9000: clock failure" << endl;
    }
    if (shmctl (segmentID, IPC_RMID, NULL) == -1)
    {
        cout << "HAL9000: clock failure" << endl;
    }
    remove ("0_template");
    system ("HAL9000Cleanup");
    usleep (SLEEP_DELAY);

    exit (0);
}

void Restart ()
{
    cout << "HAL9000: powering down ..." << endl;

    if (shmdt (theClockBase) == -1)
    {
        cout << "HAL9000: clock failure" << endl;
    }
    if (shmctl (segmentID, IPC_RMID, NULL) == -1)
    {
        cout << "HAL9000: clock failure" << endl;
    }
    remove ("0_template");
    system ("HAL9000Cleanup");
    usleep (10 * SLEEP_DELAY);

    execle ("HALstart", "HALstart", (char *) NULL, environ);
    cout << "HALstart: HAL9000 failed to restart" << endl;
    CoreDump ();

    exit (1);
}

static void SignalHandler (int signalNo, siginfo_t *info, void *context)
{
    if (signalNo == SIGRTMIN)
    {
        if (info -> si_pid == HALosPid)
        {
            messageFromHALos = 1;
        }
    }
}
