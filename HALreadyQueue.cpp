//
// HALreadyQueue.cpp
//
// Copyright (c) 2019 Robert J. Hilderman.
// All rights reserved.
//

#include "HALreadyQueue.h"

#include <cmath> // pow


// Debug output
#define DEBUG_OUTPUT false
#define DEBUG_FUNCTIONS false

#if DEBUG_OUTPUT
    #define DBO(x) do { std::cerr << "<" << __FUNCTION__ << ":" << __LINE__ << ">    "  << x; } while (false)
#else
    #define DBO(x) do { NoOp; } while (false)
#endif
#if DEBUG_FUNCTIONS
    #define DBF do { std::cerr << "<" << __FUNCTION__ << ":" << __LINE__ << ">" << endl; } while (false)
#else
    #define DBF do { NoOp; } while (false)
#endif


ReadyQueueType::ReadyQueueType (int queueSizeIn, int noOfReadyQueuesIn, cpuSchedulingPolicyCriteria* cpuSchedulingPoliciesIn)
{
    int i;

    QUEUE_SIZE = queueSizeIn;

    NO_OF_READY_QUEUES = noOfReadyQueuesIn;
    queues = new readyQueueDescriptor [NO_OF_READY_QUEUES];

    cpuSchedulingPolicies = cpuSchedulingPoliciesIn;

    for (i = 0; i < NO_OF_READY_QUEUES; i ++)
    {
        if (cpuSchedulingPolicies [i].cpuSchedulingPolicy == "ROUND_ROBIN")
        {
            queues [i].queue = new processDescriptor [QUEUE_SIZE];
            queues [i].length = 0;
            queues [i].front = QUEUE_SIZE - 1;
            queues [i].back = queues [i].front;
        }
        else if (cpuSchedulingPolicies [i].cpuSchedulingPolicy == "PRIORITY")
        {
            queues [i].queue = new processDescriptor [QUEUE_SIZE];
            queues [i].length = 0;
            queues [i].front = 0;
            queues [i].back = queues [i].front;
        }
        else if (cpuSchedulingPolicies [i].cpuSchedulingPolicy == "LOTTERY")
        {
            queues [i].queue = new processDescriptor [QUEUE_SIZE];
            queues [i].length = 0;
            queues [i].front = 0;
            queues [i].back = queues [i].front;
        }
        else if (cpuSchedulingPolicies [i].cpuSchedulingPolicy == "RANDOM")
        {
            queues [i].queue = new processDescriptor [QUEUE_SIZE];
            queues [i].length = 0;
            queues [i].front = 0;
            queues [i].back = queues [i].front;
        }
    }

    srand (TimeSeed ());
}

ReadyQueueType::~ReadyQueueType ()
{
}

int ReadyQueueType::Length (int queueNo)
{
    return queues [queueNo].length;
}

bool ReadyQueueType::IsEmpty ()
{
    int i;

    for (i = 0; i < NO_OF_READY_QUEUES; i ++)
    {
        if (queues [i].length > 0)
        {
            return false;
        }
    }

    return true;
}

bool ReadyQueueType::IsFull (int queueNo)
{
    if (queues [queueNo].length == QUEUE_SIZE)
    {
        return true;
    }

    return false;
}

string ReadyQueueType::Delete (string pid)
{
    DBF;

    int i;
    int j;
    processDescriptor process;
    bool pidFound = false;

    for (i = 0; i < NO_OF_READY_QUEUES; i ++)
    {
        // Search the ROUND_ROBIN queue
        if (cpuSchedulingPolicies [i].cpuSchedulingPolicy == "ROUND_ROBIN")
        {
            DBO ("Searching " << cpuSchedulingPolicies [i].cpuSchedulingPolicy << endl);

            j = 0;
            while (j < queues [i].length)
            {
                queues [i].front = (queues [i].front + 1) % QUEUE_SIZE;
                process = queues [i].queue [queues [i].front];
                if (pid != "")
                {
                    if (process.pid == pid)
                    {
                        pidFound = true;
                    }
                    else
                    {
                        queues [i].back = (queues [i].back + 1) % QUEUE_SIZE;
                        queues [i].queue [queues [i].back] = process;
                    }
                }
                else // (pid == "")
                {
                    if (process.type == "FOREGROUND_PROCESS")
                    {
                        pid = process.pid;
                        pidFound = true;
                    }
                    else
                    {
                        queues [i].back = (queues [i].back + 1) % QUEUE_SIZE;
                        queues [i].queue [queues [i].back] = process;
                    }
                }
                j ++;
            }
            if (pidFound)
            {
                queues [i].length --;

                DBF;
                return (pid);
            }
        }
        // Search the PRIORITY queue
        else if (cpuSchedulingPolicies [i].cpuSchedulingPolicy == "PRIORITY")
        {
            DBO ("Searching " << cpuSchedulingPolicies [i].cpuSchedulingPolicy << endl);

            for (j = 0; j < queues [i].length; ++j)
            {
                process = queues [i].queue [j];
                if (pid != "")
                {
                    if (process.pid == pid)
                    {
                        pidFound = true;
                    }
                }
                else // (pid == "")
                {
                    if (process.type == "FOREGROUND_PROCESS")
                    {
                        pid = process.pid;
                        pidFound = true;
                    }
                }
            }
            if (pidFound)
            {
                // Shift queue contents left by one
                // starting from the found process
                for (int k = j; k < queues [i].length; ++k)
                {
                    queues [i].queue [k] = queues [i].queue [k + 1];
                }

                queues [i].length --;
                queues [i].back --;

                DBF;
                return (pid);
            }
        }
        // Search the LOTTERY queue
        else if (cpuSchedulingPolicies [i].cpuSchedulingPolicy == "LOTTERY")
        {
            DBO ("Searching " << cpuSchedulingPolicies [i].cpuSchedulingPolicy << endl);

            for (j = 0; j < queues [i].length; ++j)
            {
                process = queues [i].queue [j];
                if (pid != "")
                {
                    if (process.pid == pid)
                    {
                        pidFound = true;
                    }
                }
                else // (pid == "")
                {
                    if (process.type == "FOREGROUND_PROCESS")
                    {
                        pid = process.pid;
                        pidFound = true;
                    }
                }
            }
            if (pidFound)
            {
                queues [i].length --;

                // Shift queue contents left by one
                // starting from the found process
                for (int k = j; k < queues [i].length; ++k)
                {
                    queues [i].queue [k] = queues [i].queue [k + 1];
                }

                DBF;
                return (pid);
            }
        }
        // Search the RANDOM queue
        else if (cpuSchedulingPolicies [i].cpuSchedulingPolicy == "RANDOM")
        {
            DBO ("Searching " << cpuSchedulingPolicies [i].cpuSchedulingPolicy << endl);

            for (j = 0; j < queues [i].length; ++j)
            {
                process = queues [i].queue [j];
                if (pid != "")
                {
                    if (process.pid == pid)
                    {
                        pidFound = true;
                    }
                }
                else // (pid == "")
                {
                    if (process.type == "FOREGROUND_PROCESS")
                    {
                        pid = process.pid;
                        pidFound = true;
                    }
                }
            }
            if (pidFound)
            {
                queues [i].length --;

                // Shift queue contents left by one
                // starting from the found process
                for (int k = j; k < queues [i].length; ++k)
                {
                    queues [i].queue [k] = queues [i].queue [k + 1];
                }

                DBF;
                return (pid);
            }
        }
    }

    DBF;
    return ("-1");
}

processDescriptor ReadyQueueType::SetQueueNo (processDescriptor process)
{
    DBF;

    // Do nothing if there are no other ready queues
    if (NO_OF_READY_QUEUES == 1)
    {
        return process;
    }

    // Priority process given a priority from the command line is never moved
    if (cpuSchedulingPolicies [process.queueNo].cpuSchedulingPolicy == "PRIORITY" &&
        process.priority != NOT_A_PRIORITY_PROCESS)
    {
        DBO ("PID[" << process.pid << "]    PRIORITY - Immovable process" << endl);
        DBF;
        return process;
    }

    // Non priority processes are always moved from the priority queue
    else if (cpuSchedulingPolicies [process.queueNo].cpuSchedulingPolicy == "PRIORITY" &&
        process.priority == NOT_A_PRIORITY_PROCESS)
    {
        --process.interruptCounter;

        // Processes moving down the queues
        if (process.interruptCounter == 0 && process.direction == "DOWN")
        {
            DBO ("PID[" << process.pid << "]    Moved down    From: "
                << cpuSchedulingPolicies [process.queueNo].cpuSchedulingPolicy
                << "    To: " << cpuSchedulingPolicies [process.queueNo + 1].cpuSchedulingPolicy
                << endl);

            ++process.queueNo;

            // In the lowest queue
            if (cpuSchedulingPolicies[process.queueNo].interruptsUntilMoveDown == -1)
            {
                DBO ("PID[" << process.pid << "]    Direction changed to up" << endl);

                // Start moving up
                process.direction = "UP";
                process.interruptCounter = cpuSchedulingPolicies[process.queueNo].interruptsUntilMoveUp;
            }
            else
            {
                process.interruptCounter = cpuSchedulingPolicies[process.queueNo].interruptsUntilMoveDown;
            }
        }
        // Processes moving up the queues
        else if (process.interruptCounter == 0 && process.direction == "UP")
        {
            DBO ("PID[" << process.pid << "]    Moved up    From: "
                << cpuSchedulingPolicies [process.queueNo].cpuSchedulingPolicy
                << "    To: " << cpuSchedulingPolicies [process.queueNo - 1].cpuSchedulingPolicy
                << endl);

            --process.queueNo;

            // In the highest queue
            if (cpuSchedulingPolicies[process.queueNo].interruptsUntilMoveUp == -1)
            {
                DBO ("PID[" << process.pid << "]    Direction changed to down" << endl);

                // Start moving down
                process.direction = "DOWN";
                process.interruptCounter = cpuSchedulingPolicies[process.queueNo].interruptsUntilMoveDown;
            }
            else
            {
                process.interruptCounter = cpuSchedulingPolicies[process.queueNo].interruptsUntilMoveUp;
            }
        }

        DBF;
        return process;
    }

    // Other policies move when their interrupt limit is reached
    else
    {
        --process.interruptCounter;

        // Processes moving down the queues
        if (process.interruptCounter == 0 && process.direction == "DOWN")
        {
            DBO ("PID[" << process.pid << "]    Moved down    From: "
                << cpuSchedulingPolicies [process.queueNo].cpuSchedulingPolicy
                << "    To: " << cpuSchedulingPolicies [process.queueNo + 1].cpuSchedulingPolicy
                << endl);

            ++process.queueNo;

            // In the lowest queue
            if (cpuSchedulingPolicies[process.queueNo].interruptsUntilMoveDown == -1)
            {
                DBO ("PID[" << process.pid << "]    Direction changed to up" << endl);

                // Start moving up
                process.direction = "UP";
                process.interruptCounter = cpuSchedulingPolicies[process.queueNo].interruptsUntilMoveUp;
            }
            else
            {
                process.interruptCounter = cpuSchedulingPolicies[process.queueNo].interruptsUntilMoveDown;
            }
        }
        // Processes moving up the queues
        else if (process.interruptCounter == 0 && process.direction == "UP")
        {
            DBO ("PID[" << process.pid << "]    Moved up    From: "
                << cpuSchedulingPolicies [process.queueNo].cpuSchedulingPolicy
                << "    To: " << cpuSchedulingPolicies [process.queueNo - 1].cpuSchedulingPolicy
                << endl);

            --process.queueNo;

            // In the highest queue
            if (cpuSchedulingPolicies[process.queueNo].interruptsUntilMoveUp == -1)
            {
                DBO ("PID[" << process.pid << "]    Direction changed to down" << endl);

                // Start moving down
                process.direction = "DOWN";
                process.interruptCounter = cpuSchedulingPolicies[process.queueNo].interruptsUntilMoveDown;
            }
            else
            {
                process.interruptCounter = cpuSchedulingPolicies[process.queueNo].interruptsUntilMoveUp;
            }
        }
    }

    DBF;
    return process;
}

void ReadyQueueType::Enqueue (processDescriptor process)
{
    DBF;

    // Round Robin - Circular queue
    if (cpuSchedulingPolicies [process.queueNo].cpuSchedulingPolicy == "ROUND_ROBIN")
    {
        DBO ("PID[" << process.pid << "]    Enqueueing in: "
            << cpuSchedulingPolicies [process.queueNo].cpuSchedulingPolicy
            << endl);

        queues [process.queueNo].back = (queues [process.queueNo].back + 1) % QUEUE_SIZE;
        queues [process.queueNo].queue [queues [process.queueNo].back] = process;
    }

    // Priority - Sorted list
    else if (cpuSchedulingPolicies [process.queueNo].cpuSchedulingPolicy == "PRIORITY")
    {
        DBO ("PID[" << process.pid << "]    Enqueueing in: "
            << cpuSchedulingPolicies [process.queueNo].cpuSchedulingPolicy
            << endl);

        processDescriptor* processQueue = queues [process.queueNo].queue;
        int length = queues [process.queueNo].length;

        // Add all processes of higher or equal priority to new array,
        // then the new process, then the lower priority processes
        // + 1: current + new
        processDescriptor* temp = new processDescriptor[length + 1];
        int i = 0;
        while (i < length && processQueue[i].priority <= process.priority)
        {
            temp[i] = processQueue[i];
            ++i;
        }
        temp[i] = process;
        while (i < length)
        {
            temp[i+1] = processQueue[i];
            ++i;
        }

        // Copy the contents
        for (int i = 0; i < length + 1; ++i)
        {
            processQueue[i] = temp[i];
        }

        delete[] temp;
    }

    // Lottery - Unsorted list
    else if (cpuSchedulingPolicies [process.queueNo].cpuSchedulingPolicy == "LOTTERY")
    {
        DBO ("PID[" << process.pid << "]    Enqueueing in: "
            << cpuSchedulingPolicies [process.queueNo].cpuSchedulingPolicy
            << endl);
        
        // Always add the process to the end of the queue
        queues [process.queueNo].queue [queues [process.queueNo].back] = process;
        queues [process.queueNo].back = (queues [process.queueNo].back + 1);
    }

    // Random - Unsorted list
    else if (cpuSchedulingPolicies [process.queueNo].cpuSchedulingPolicy == "RANDOM")
    {
        DBO ("PID[" << process.pid << "]    Enqueueing in: "
            << cpuSchedulingPolicies [process.queueNo].cpuSchedulingPolicy
            << endl);

        queues [process.queueNo].queue [queues [process.queueNo].back] = process;
        queues [process.queueNo].back = (queues [process.queueNo].back + 1);
    }

    else
    {
        cout << "<" << __LINE__ << ":" << __FUNCTION__ << "> invalid cpu scheduling policy" << endl;
        exit (1);
    }

    queues [process.queueNo].length ++;

    DBF;
    return;
}

processDescriptor ReadyQueueType::Dequeue ()
{
    DBF;

    processDescriptor process;

    for (int i = 0; i < NO_OF_READY_QUEUES; i ++)
    {
        if (queues [i].length > 0)
        {
            if (cpuSchedulingPolicies [i].cpuSchedulingPolicy == "ROUND_ROBIN")
            {
                DBO ("PID["
                    << (queues [i].queue [queues [i].front].pid == "" ? "NULL" : queues [i].queue [queues [i].front].pid)
                    << "]    Dequeueing from: "
                    << cpuSchedulingPolicies [i].cpuSchedulingPolicy
                    << endl);

                queues [i].front = (queues [i].front + 1) % QUEUE_SIZE;
                queues [i].length --;

                DBF;
                return (queues [i].queue [queues [i].front]);
            }
            else if (cpuSchedulingPolicies [i].cpuSchedulingPolicy == "PRIORITY")
            {
                DBO ("PID["
                    << (queues [i].queue [queues [i].front].pid == "" ? "NULL" : queues [i].queue [queues [i].front].pid)
                    << "]    Dequeueing from: "
                    << cpuSchedulingPolicies [i].cpuSchedulingPolicy
                    << endl);
                
                processDescriptor dequeuedProcess = queues [i].queue [0];

                queues [i].length --;

                // Shift queue contents left by one
                for (int j = 0; j < queues [i].length; ++j)
                {
                    queues [i].queue [j] = queues [i].queue [j + 1];
                }

                DBF;
                return (dequeuedProcess);
            }
            else if (cpuSchedulingPolicies [i].cpuSchedulingPolicy == "LOTTERY")
            {
                int noOfTicketsForProcess = 0;
                int maxNoOfTickets = 0;

                // Allocate the tickets based on process priority
                for (int j = 0; j < queues [i].back; ++j)
                {
                    if (queues [i].queue [j].priority == NOT_A_PRIORITY_PROCESS)
                    {
                        noOfTicketsForProcess = 1;
                    }
                    else
                    {
                        // 2^(10-priority-1)
                        noOfTicketsForProcess = pow (2, (NOT_A_PRIORITY_PROCESS - queues [i].queue [j].priority - 1));
                    }
                    maxNoOfTickets += noOfTicketsForProcess;
                }

                // Get the winning ticket number
                int winningTicketNo = GetWinningTicketNo (maxNoOfTickets);
                processDescriptor winningProcess;
                maxNoOfTickets = 0;

                // Find the winning process
                for (int j = 0; j < queues [i].back; ++j)
                {
                    if (queues [i].queue [j].priority == NOT_A_PRIORITY_PROCESS)
                    {
                        noOfTicketsForProcess = 1;
                    }
                    else
                    {
                        // 2^(10-priority-1)
                        noOfTicketsForProcess = pow (2, (NOT_A_PRIORITY_PROCESS - queues [i].queue [j].priority - 1));
                    }
                    maxNoOfTickets += noOfTicketsForProcess;

                    // Found the winning process
                    if (winningTicketNo <= maxNoOfTickets)
                    {
                        winningProcess = queues [i].queue [j];

                        // Shift processes to fill the gap
                        for (int k = j; k < queues [i].back - 1; ++k)
                        {
                            queues [i].queue [k] = queues [i].queue [k + 1];
                        }

                        queues [i].back --;
                        queues [i].length --;

                        break;
                    }
                }

                DBO ("PID[" << winningProcess.pid << "]    Dequeueing from: "
                    << cpuSchedulingPolicies [i].cpuSchedulingPolicy << endl);

                DBF;
                return (winningProcess);
            }
            // RANDOM - Select a process at random from the ready queue
            //        - All processes have an equal chance of being selected
            else if (cpuSchedulingPolicies [i].cpuSchedulingPolicy == "RANDOM")
            {
                // Get a random number from 0 to (length - 1)
                int winningProcess = (GetWinningTicketNo (queues [i].length) - 1);
                processDescriptor chosenProcess;

                chosenProcess = queues [i].queue [winningProcess];

                // Shift processes to fill the gap
                for (int j = winningProcess; j < queues [i].back - 1; ++j)
                {
                    queues [i].queue [j] = queues [i].queue [j + 1];
                }

                queues [i].back --;
                queues [i].length --;

                DBO ("PID[" << chosenProcess.pid << "]    Dequeueing from: "
                    << cpuSchedulingPolicies [i].cpuSchedulingPolicy << endl);

                DBF;
                return(chosenProcess);
            }
            else
            {
                cout << "<" << __LINE__ << ":" << __FUNCTION__ << "> Invalid cpu scheduling policy" << endl;
                exit (1);
            }
        }
    }
}

unsigned int ReadyQueueType::TimeSeed ()
{
    time_t now;
    unsigned char *p;
    unsigned seed;
    size_t i;

    now = time (0);
    p = (unsigned char *) &now;
    seed = 0;

    for (i = 0; i < sizeof (now); i ++)
    {
        seed = seed * (UCHAR_MAX + 2U) + p [i];
    }

    return seed;
}

int ReadyQueueType::GetWinningTicketNo (int maxLotteryTicketNo)
{
    DBF;

    double randomNumber;
    int randomNumberScaled;

    randomNumber = rand () / (1.0 + (double) RAND_MAX);
    randomNumberScaled = (int) (randomNumber * (double) maxLotteryTicketNo) + 1;

    DBF;
    return randomNumberScaled;
}

// See what happens when PROCESS_DONE
