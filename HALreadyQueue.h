//
// HALreadyQueue.h
//
// Copyright (c) 2019 Robert J. Hilderman.
// All rights reserved.
//

#include <iostream>
#include <math.h>
#include <stdlib.h>
#include <time.h>
#include <limits.h>

using namespace std;

#define HAL_READY_QUEUE_H

#ifndef HAL_GLOBALS_H
    #include "HALglobals.h"
#endif

#ifndef HAL_PROCESS_DESCRIPTOR_H
    #include "HALprocessDescriptor.h"
#endif

#ifndef HAL_CPU_SCHEDULING_POLICY_CRITERIA_H
    #include "HALcpuSchedulingPolicyCriteria.h"
#endif

extern string itos (int i);

struct readyQueueDescriptor
{
    processDescriptor *queue;
    int front;
    int back;
    int length;
};

class ReadyQueueType
{
public:
    ReadyQueueType (int queueSizeIn, int noOfReadyQueuesIn, cpuSchedulingPolicyCriteria *cpuSchedulingPoliciesIn);
    ~ReadyQueueType ();
    int Length (int queueNo);
    bool IsEmpty ();
    bool IsFull (int queueNo);
    string Delete (string pid);
    processDescriptor SetQueueNo (processDescriptor process);
    void Enqueue (processDescriptor process);
    processDescriptor Dequeue ();
private:
    readyQueueDescriptor *queues;
    cpuSchedulingPolicyCriteria *cpuSchedulingPolicies;
    int QUEUE_SIZE;
    int NO_OF_READY_QUEUES;
    unsigned int TimeSeed ();
    int GetWinningTicketNo (int maxTicketNo);
};
