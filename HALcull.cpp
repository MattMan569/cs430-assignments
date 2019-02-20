//
// HALcull.cpp
//
// Copyright (c) 2019 Robert J. Hilderman.
// All Rights Reserved.
//

#include "HALcull.h"

void Cull (string command, string arguments [])
{
    string targetPid;

    if (arguments [1] != "")
    {
        SendReturnStatusToHALshell (itos (nextPid), "", "cull requires a maximum of one argument (the process id)");
        nextPid ++;
        return;
    }

    targetPid = arguments [0];

    // The "whole" system must be idle before culling a process.
    // First, stop any executing process.
    PauseAnyExecutingProcess (command);

    // Second, consume any undelivered messages from the HAL9000.
    ConsumeUndeliveredMessages ();

    // Third, wait for any display output and disk input/output requests
    // to finish.
    WaitForIoRequestsToFinish ();

    // Then ...
    SearchAndDestroy (targetPid);

    return;
}

void PauseAnyExecutingProcess (string command)
{
    processDescriptor process;
    int i;

    process = NullProcess ();

    process.pid = itos (0);
    process.type = "";
    process.action = "PAUSE_ANY_EXECUTING_PROCESS";
    process.command = command;
    for (i = 0; i < MAX_COMMAND_LINE_ARGUMENTS; i ++)
    {
        process.arguments [i] = "";
    }
    process.returnValue = "";
    process.partitionNo = 0;
    SendMessageToHAL9000 (process);

    return;
}

void ConsumeUndeliveredMessages ()
{
    do
    {
        BlockSignals ("HALos");
        if (messageFromHAL9000)
        {
            messageFromHAL9000 = 0;
            GetMessageFromHAL9000 ();
            if (HAL9000Message.type == "INTERRUPT")
            {
                HandleHAL9000Interrupt ();
            }
            else if (HAL9000Message.type == "SYSTEM_CALL")
            {
                HandleSystemCall ();
            }
        }
        UnblockSignals ("HALos");
    } while (HAL9000Message.type != "EXECUTING_PROCESS_PAUSED" && HAL9000Message.type != "NO_EXECUTING_PROCESS");

    if (HAL9000Message.type == "EXECUTING_PROCESS_PAUSED")
    {
        cpuProcess.runningTime = cpuProcess.runningTime
            + atoi (HAL9000Message.parameter7.c_str ());
    }

    return;
}

void WaitForIoRequestsToFinish ()
{
    while (displayProcess.pid != itos (-1) || diskProcess.pid != itos (-1))
    {
        BlockSignals ("HALos");
        if (messageFromHALdisplayDriver)
        {
            messageFromHALdisplayDriver = 0;
            GetMessageFromHALdisplayDriver ();
            SetSystemCallParameters (displayProcess, "", "", "", "", "", "");
            readyQueue.Enqueue (displayProcess);
            displayProcess = NullProcess ();
            ClearMessageParameters (HALdisplayDriverMessage);
        }
        else if (messageFromHALdiskDriver)
        {
            messageFromHALdiskDriver = 0;
            GetMessageFromHALdiskDriver ();
            if (diskProcess.systemCall == "OPEN")
            {
                if (HALdiskDriverMessage.parameter5 == "FILE_OPEN_OK")
                {
                    if (!diskProcess.fileTable.Find (diskProcess.systemCallParameter1, diskProcess.systemCallParameter2))
                    {  
                        diskProcess.fileTable.Insert (diskProcess.systemCallParameter1, diskProcess.systemCallParameter2, diskProcess.systemCallParameter3);
                    }
                    else 
                    {
                        HALdiskDriverMessage.parameter5 = "FILE_ALREADY_OPEN";
                    }
                }
                SetSystemCallParameters (diskProcess, "", "", "", "", "", HALdiskDriverMessage.parameter5);
            }
            else if (diskProcess.systemCall == "READ")
            {
                diskProcess.fileTable.Find (diskProcess.systemCallParameter1, diskProcess.systemCallParameter2);
                diskProcess.fileTable.Write (HALdiskDriverMessage.parameter3);
                SetSystemCallParameters (diskProcess, "", "", "", "", HALdiskDriverMessage.parameter4, HALdiskDriverMessage.parameter5);
            }
            else if (diskProcess.systemCall == "WRITE" || diskProcess.systemCall == "NEWLINE")
            {
                SetSystemCallParameters (diskProcess, "", "", "", "", "", HALdiskDriverMessage.parameter5);
            }
            readyQueue.Enqueue (diskProcess);
            diskProcess = NullProcess ();
            ClearMessageParameters (HALdiskDriverMessage);
        }
        UnblockSignals ("HALos");
    }

    return;
}

void SearchAndDestroy (string targetPid)
{
    string pid;

    BlockSignals ("HALos");
    // If the cull command was entered on the command line in HALshell,
    // then targetPid will contain the pid of a background process.
    if (targetPid.length () > 0)
    {
        // If the process being culled is the one that was running
        // when the cull command was received, then cull it.
        if (cpuProcess.pid != itos (-1) &&
            cpuProcess.pid == targetPid)
        {
            ExpungeProcess ("BACKGROUND_PROCESS", cpuProcess.pid);
            cpuProcess = NullProcess ();
        }
        else
        {
            // Otherwise, find and remove the process with pid == targetPid.
            // It could be in the ready queue. If found there, cull it.
            pid = readyQueue.Delete (targetPid);
            if (pid != itos (-1))
            {
                ExpungeProcess ("BACKGROUND_PROCESS", pid);
            }
            else
            {
                // Otherwise, it could be in a device queue.
                CheckInDeviceQueues ("BACKGROUND_PROCESS", targetPid);
            }
            // If a process was paused, restart it.
            if (cpuProcess.pid != itos (-1))
            {
                RestartCpuProcess ();
            }
        }
    }
    // If the cull command was generated by pressing ctrl/c in HALshell,
    // then targetPid will be blank. This means that the process to be
    // culled is the foreground process.
    else // (targetPid.length () == 0)
    {
        // If it is the current keyboard process, cull it.
        if (keyboardProcess.pid != itos (-1) &&
            keyboardProcess.type == "FOREGROUND_PROCESS")
        {
            ExpungeProcess ("FOREGROUND_PROCESS", keyboardProcess.pid);
            if (keyboardQueue.IsEmpty ())
            {
                keyboardProcess = NullProcess ();
            }
            else
            {
                keyboardProcess = keyboardQueue.Dequeue ();
                SendMessageToHALkeyboardDriver (keyboardProcess.pid, keyboardProcess.systemCall);
            }
            // If a process was paused, restart it.
            if (cpuProcess.pid != itos (-1))
            {
                RestartCpuProcess ();
            }
        }    
        // Or, if the process being culled is the one that was running
        // when the cull command was received, then cull it.
        else if (cpuProcess.pid != itos (-1) &&
                 cpuProcess.type == "FOREGROUND_PROCESS")
        {
            ExpungeProcess ("FOREGROUND_PROCESS", cpuProcess.pid);
            cpuProcess = NullProcess ();
        }
        else
        {
            // Otherwise, find and remove the foreground process.
            // It could be in the ready queue. If found there, cull it.
            pid = readyQueue.Delete (targetPid);
            if (pid != itos (-1))
            {
                ExpungeProcess ("FOREGROUND_PROCESS", pid);
            }
            else
            {
                // Otherwise, it could be in a device queue.
                CheckInDeviceQueues ("FOREGROUND_PROCESS", targetPid);
            }
            // If a process was paused, restart it.
            if (cpuProcess.pid != itos (-1))
            {
                RestartCpuProcess ();
            }
        }
    }
    RestartDeviceQueues ();
    UnblockSignals ("HALos");

    return;
}

void ExpungeProcess (string type, string targetPid)
{
    processDescriptor process;

    process = NullProcess ();
    process.pid = targetPid;
    process.action = "PROCESS_CULLED";
    process.partitionNo = UpdatePartitionTable (process.pid, process.action);
    remove ((targetPid + "_backingstore").c_str ());
    if (type == "FOREGROUND_PROCESS")
    {
        SendReturnStatusToHALshell ("", "ok", "process " + targetPid + " culled");
    }
    else // (type == "BACKGROUND_PROCESS")
    {
        SendReturnStatusToHALshell (itos (nextPid), "ok", "process " + targetPid + " culled");
        nextPid ++;
    }

    return;
}

void RestartCpuProcess ()
{
    if (HAL9000Message.type == "EXECUTING_PROCESS_PAUSED")
    {
        cpuProcess.action = "CONTINUE_EXECUTING_PROCESS";
        cpuProcess.partitionNo = UpdatePartitionTable (cpuProcess.pid, cpuProcess.action);
    }
    SendMessageToHAL9000 (cpuProcess);

    ClearMessageParameters (HAL9000Message);

    return;
}

void RestartDeviceQueues ()
{
    fileDescriptor file;

    if (!keyboardQueue.IsEmpty ())
    {
        keyboardProcess = keyboardQueue.Dequeue ();
        SendMessageToHALkeyboardDriver (keyboardProcess.pid, keyboardProcess.systemCall);
    }
    if (!displayQueue.IsEmpty ())
    {
        displayProcess = displayQueue.Dequeue ();
        SendMessageToHALdisplayDriver (displayProcess.pid, displayProcess.systemCall, displayProcess.systemCallParameter1);
    }
    if (!diskQueue.IsEmpty ())
    {
        diskProcess = diskQueue.Dequeue ();
        if (diskProcess.systemCall == "OPEN")
        {
            SendMessageToHALdiskDriver (diskProcess.pid, diskProcess.systemCall, diskProcess.systemCallParameter1, diskProcess.systemCallParameter3, "", "");
        }
        else // (diskProcess.systemCall == "READ" || "WRITE" || "NEWLINE")
        {
            if (diskProcess.systemCall == "READ")
            {
                diskProcess.fileTable.Find (diskProcess.systemCallParameter1, diskProcess.systemCallParameter2);
            }
            else // diskProcess.systemCall == "WRITE" || "NEWLINE")
            {
                diskProcess.fileTable.Find (diskProcess.systemCallParameter2, diskProcess.systemCallParameter3);
            }              
            file = diskProcess.fileTable.Read ();
            if (file.mode == "input")
            {
                SendMessageToHALdiskDriver (diskProcess.pid, diskProcess.systemCall, file.name, file.mode, file.markerPosition, "");
            }
            else
            {
                SendMessageToHALdiskDriver (diskProcess.pid, diskProcess.systemCall, file.name, file.mode, "", diskProcess.systemCallParameter1);
            }
        }
    }

    return;
}

void CheckInDeviceQueues (string type, string targetPid)
{
    string result;

    // Check in the display queue. If found there, cull it.
    if (ProcessInDeviceQueue (displayQueue, type, targetPid))
    {
        ExpungeProcess (type, targetPid);
    }
    // Check in the disk queue. If found there, cull it.
    else if (ProcessInDeviceQueue (diskQueue, type, targetPid))
    {
        ExpungeProcess (type, targetPid);
    }
    // Check in the keyboard queue. If found there, cull it.
    else if (ProcessInDeviceQueue (keyboardQueue, type, targetPid))
    {
        ExpungeProcess (type, targetPid);
    }
    // Or it may not exist.
    else
    {
        if (type == "FOREGROUND_PROCESS")
        {
            SendReturnStatusToHALshell ("", "", "process " + targetPid + " not found");
        }
        else // (type == "BACKGROUND_PROCESS")
        {
            if (targetPid == itos (nextPid))
            {
                SendReturnStatusToHALshell (itos (nextPid), "ok", "process " + targetPid + " culled");
                nextPid ++;
            }
            else
            {
                SendReturnStatusToHALshell (itos (nextPid), "", "process " + targetPid + " not found");
                nextPid ++;
            }
        }
    }

    return;
}

bool ProcessInDeviceQueue (QueueType &queue, string type, string &targetPid)
{
    int i;
    int queueLength;
    bool processFound = false;
    processDescriptor process;

    process = NullProcess ();

    queueLength = queue.Length ();
    for (i = 0; i < queueLength; i ++)
    {
        if (type == "BACKGROUND_PROCESS")
        {
            process = queue.Dequeue ();
            if (process.pid == targetPid)
            {
                processFound = true;
            }
            else
            {
                queue.Enqueue (process);
            }
        }
        else // (type == "FOREGROUND_PROCESS")
        {
            process = queue.Dequeue ();
            if (process.type == "FOREGROUND_PROCESS")
            {
                processFound = true;
                targetPid = process.pid;
            }
            else
            {
                queue.Enqueue (process);
            }
        }
    }

    return processFound;
}
