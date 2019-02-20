//
// HALmemory.cpp
//
// Copyright (c) 2017 Robert J. Hilderman.
// All Rights Reserved.
//

#include "HALmemory.h"

MemoryType::MemoryType (int partitionSize, int noOfPartitions)
{
    int partitionNo;

    PARTITION_SIZE = partitionSize;
    NO_OF_PARTITIONS = noOfPartitions;

    memory = new memoryCell* [PARTITION_SIZE];

    for (p = 0; p < PARTITION_SIZE; p ++)
    {
        memory [p] = new memoryCell [NO_OF_PARTITIONS];
    }

    for (p = 0; p < PARTITION_SIZE; p ++)
    {
        for (partitionNo = 0; partitionNo < NO_OF_PARTITIONS; partitionNo ++)
        {
            memory [p][partitionNo].symbol = "";
            memory [p][partitionNo].value = "";
        }
    }
}

MemoryType::~MemoryType ()
{
}

void MemoryType::ResetP ()
{
    p = -1;

    return;
}

void MemoryType::IterateUp ()
{
    p ++;

    return;
}

void MemoryType::IterateDown ()
{
    p --;

    return;
}

int MemoryType::GetP ()
{
    return p;
}

void MemoryType::SetP (int q)
{
    p = q;

    return;
}

memoryCell MemoryType::Read (int partitionNo)
{
    memoryCell contents;

    contents.symbol = memory [p][partitionNo].symbol;
    contents.value = memory [p][partitionNo].value;

    return contents;
}

void MemoryType::Write (string symbol, string value, int partitionNo)
{
    memory [p][partitionNo].symbol = symbol;
    memory [p][partitionNo].value = value;

    return;
}

void MemoryType::ReWrite (string symbol, string value, int partitionNo)
{
    if (symbol.length () > 0)
    {
        memory [p][partitionNo].symbol = symbol;
    }
    memory [p][partitionNo].value = value;

    return;
}

void MemoryType::Push (string symbol, string value, int partitionNo)
{
    p --;
    memory [p][partitionNo].symbol = symbol;
    memory [p][partitionNo].value = value;

    return;
}

memoryCell MemoryType::Pop (int partitionNo)
{
    memoryCell contents;

    contents.symbol = memory [p][partitionNo].symbol;
    contents.value = memory [p][partitionNo].value;
    
    memory [p][partitionNo].symbol = "";
    memory [p][partitionNo].value = "";

    p ++;

    return contents;
}

void MemoryType::Clear (int startAddress, int partitionNo)
{
    for (p = startAddress; p < PARTITION_SIZE; p ++)
    {
        memory [p][partitionNo].symbol = "";
        memory [p][partitionNo].value = "";
    }

    return;
}
