//
// HALqueue.h
//
// Copyright (c) 2019 Robert J. Hilderman.
// All Rights Reserved.
//

#define HAL_QUEUE_H

#ifndef HAL_PROCESS_DESCRIPTOR_H
    #include "HALprocessDescriptor.h"
#endif

class QueueType
{
public:
    QueueType (int queueSize);
    ~QueueType ();
    int Length ();
    bool IsEmpty ();
    bool IsFull ();
    void Enqueue (processDescriptor process);
    processDescriptor Dequeue ();
private:
    processDescriptor *queue;
    int front;
    int back;
    int length;
    int QUEUE_SIZE;
};
