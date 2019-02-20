//
// HALpartitionTable.cpp
//
// Copyright (c) 2019 Robert J. Hilderman.
// All rights reserved.
//

#include "HALpartitionTable.h"

PartitionTableType::PartitionTableType (int partitionTableSize)
{
    PARTITION_TABLE_SIZE = partitionTableSize;
    partitionTable = new partitionDescriptor [PARTITION_TABLE_SIZE];

    for (p = 0; p < PARTITION_TABLE_SIZE; p ++)
    {
        partitionTable [p].pid = 0;
        partitionTable [p].age = 0;
    }

    p = -1;
}

PartitionTableType::~PartitionTableType ()
{
}

void PartitionTableType::ResetP ()
{
    p = -1;

    return;
}

void PartitionTableType::Iterate ()
{
    p ++;

    return;
}

int PartitionTableType::GetP ()
{
    return p;
}

void PartitionTableType::SetP (int q)
{
    p = q;

    return;
}

int PartitionTableType::Size ()
{
    return PARTITION_TABLE_SIZE;
}

partitionDescriptor PartitionTableType::Read ()
{
    return (partitionTable [p]);
}

void PartitionTableType::IncrementAges ()
{
    for (p = 0; p < PARTITION_TABLE_SIZE; p ++)
    {
        if (partitionTable [p].pid != 0)
        {
            partitionTable [p].age ++;
        }
    }

    return;
}

void PartitionTableType::Insert (int pid)
{
    partitionTable [p].pid = pid;
    partitionTable [p].age = 0;

    return;
}

bool PartitionTableType::Find (int pid)
{
    p = 0;
    while (p < PARTITION_TABLE_SIZE)
    {
        if (pid == partitionTable [p].pid)
        {
            return true;
        }
        p ++;
    }

    return false;
}

void PartitionTableType::FindOldest ()
{
    int age = -1;
    int q;

    p = 0;
    while (p < PARTITION_TABLE_SIZE)
    {
        if (partitionTable [p].age > age)
        {
            age = partitionTable [p].age;
            q = p;
        }
        p ++;
    }
    p = q;

    return;
}

void PartitionTableType::Delete ()
{
    partitionTable [p].pid = 0;
    partitionTable [p].age = 0; 

    return;
}
