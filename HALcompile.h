//
// HALcompile.h
//
// Copyright (c) 2019 Robert J. Hilderman.
// All Rights Reserved.
//

#include <iostream>
#include <fstream>
#include <string>
#include <stdlib.h>

using namespace std;

#ifndef HAL_MEMORY_H
    #include "HALmemory.h"
#endif

extern MemoryType ram;
extern int nextPid;
extern int QUANTUM_LENGTH;
extern int PARTITION_SIZE;

extern void SendReturnStatusToHALshell (string pid, string returnValue, string message);

void Compile (string arguments []);
bool GetProcessImageTemplate ();
string Push (memoryCell contents);
string Set (memoryCell contents);
string DetermineMemoryCellContentsForReturnAndPushCommands (memoryCell contents1, memoryCell &contents2);
string ProcessImageToFile (string programName);
bool AddressField (string symbol, int i, int globalSymbolsTableStartAddress, int globalSymbolsTableEndAddress, int functionCallStackStartAddress, int functionCallStackEndAddress);
void SetKernelVariableValue (string kernelVariableDescription, string value);
int GetMemorySegmentBoundary (string segmentStartAddressDescription, int &segmentSize);
string AllocateGlobalSymbol (memoryCell contents, int currentProgramTextAddress);
string AssignTypeToGlobalSymbol (memoryCell contents);
string AllocateGlobalArray (memoryCell contents);
int GetKernelVariableIntegerValue (string kernelVariableDescription);
int GetGlobalSymbolAddress (string value);
string GetDataType (string dataType, string &result);
string GetDataTypeCategory (string dataTypeCategory, string &result);
string ValidateSymbol (string symbol);
string ValidateValue (string value);
bool IsInteger (string value);
bool IsFloat (string value);

extern string itos (int i);
