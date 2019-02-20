//
// HALmemory.h
//
// Copyright (c) 2017 Robert J. Hilderman.
// All Rights Reserved.
//

#define HAL_MEMORY_H

#include <string>

using namespace std;

#ifndef HAL_GLOBALS_H
    #include "HALglobals.h"
#endif

struct memoryCell
{
    string symbol;
    string value;
};

class MemoryType
{
public:
    MemoryType (int partitionSize, int noOfPartition);
    ~MemoryType ();
    void ResetP ();
    void IterateUp ();
    void IterateDown ();
    int GetP ();
    void SetP (int q);
    memoryCell Read (int partitionNo);
    void Write (string symbol, string value, int partitionNo);
    void ReWrite (string symbol, string value, int partitionNo);
    void Push (string symbol, string value, int partitionNo);
    memoryCell Pop (int partitionNo);
    void Clear (int startAddress, int partitionNo);
private:
    memoryCell** memory;
    int p;
    int PARTITION_SIZE;
    int NO_OF_PARTITIONS;
};
