//
// HALqueue.cpp
//
// Copyright (c) 2019 Robert J. Hilderman.
// All Rights Reserved.
//

#include "HALqueue.h"

QueueType::QueueType (int queueSize)
{
    QUEUE_SIZE = queueSize;
    queue = new processDescriptor [QUEUE_SIZE];

    length = 0;
    front = QUEUE_SIZE - 1;
    back = front;
}

QueueType::~QueueType ()
{
}

int QueueType::Length ()
{
    return length;
}

bool QueueType::IsEmpty ()
{
    if (length == 0)
    {
        return true;
    }

    return false;
}

bool QueueType::IsFull ()
{
    if (length == QUEUE_SIZE)
    {
        return true;
    }

    return false;
}

void QueueType::Enqueue (processDescriptor process)
{
    back = (back + 1) % QUEUE_SIZE;
    queue [back] = process;

    length ++;

    return;
}

processDescriptor QueueType::Dequeue ()
{
    front = (front + 1) % QUEUE_SIZE;

    length --;

    return (queue [front]);
}
