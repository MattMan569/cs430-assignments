//
// HALpartitionTable.h
//
// Copyright (c) 2019 Robert J. Hilderman.
// All rights reserved.
//

#define HAL_PARTITION_TABLE_H

#include <string>

using namespace std;

#ifndef HAL_GLOBALS_H
    #include "HALglobals.h"
#endif

struct partitionDescriptor
{
    int pid;
    int age;
};

class PartitionTableType
{
public:
    PartitionTableType (int PartitionTableSize);
    ~PartitionTableType ();
    void ResetP ();
    void Iterate ();
    int GetP ();
    void SetP (int q);
    int Size ();
    partitionDescriptor Read ();
    void IncrementAges ();
    void Insert (int pid);
    bool Find (int pid);
    void FindOldest ();
    void Delete ();
private:
    partitionDescriptor *partitionTable;
    int p;
    int PARTITION_TABLE_SIZE;
};
