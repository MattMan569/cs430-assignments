//
// HAL9000.cpp
//
// Copyright (c) 2019 Robert J. Hilderman.
// All Rights Reserved.
//

#include "HAL9000.h"

// Debug output
#define DEBUG_OUTPUT false
#define DEBUG_FUNCTIONS false

#if DEBUG_OUTPUT
#define DBO(x)                                                               \
    do                                                                       \
    {                                                                        \
        std::cerr << "<" << __FUNCTION__ << ":" << __LINE__ << ">    " << x; \
    } while (false)
#else
#define DBO(x) \
    do         \
    {          \
        NoOp;  \
    } while (false)
#endif
#if DEBUG_FUNCTIONS
#define DBF                                                                 \
    do                                                                      \
    {                                                                       \
        std::cerr << "<" << __FUNCTION__ << ":" << __LINE__ << ">" << endl; \
    } while (false)
#else
#define DBF   \
    do        \
    {         \
        NoOp; \
    } while (false)
#endif

// The "Processor"

void HAL9000()
{
    string action;
    int pid;
    string command;
    string arguments[MAX_COMMAND_LINE_ARGUMENTS];
    string buffer;
    string result;
    int quantumLength;
    bool systemCall;
    string instructionPointer = "0d_null";
    string returnValue;
    int runningTime;
    int endTime;

    do
    {
        BlockSignals("HAL9000");
        if (instructionPointer != "0d_null" && GetKernelVariableIntegerValue("QUANTUM_TIME_REMAINING") == 0)
        {
            runningTime = GetKernelVariableIntegerValue("RUNNING_TIME");
            endTime = GetClockTicks();
            //
            //ProcessImageToFile (pid, "backingstore");
            //
            SendMessageToHALos("INTERRUPT", "QUANTUM_EXPIRED", itos(pid), "", "", "", itos(endTime), itos(runningTime));
            instructionPointer = "0d_null";
        }

        if (messageFromHALos)
        {
            DBF;

            messageFromHALos = 0;
            action = GetMessageFromHALos(pid, command, arguments, buffer, result, quantumLength);

            if (action == "EXECUTE_NEW_PROCESS_IN_UNUSED_PARTITION")
            {
                DBO("Executing new process [" << pid << "]    In unused partition: " << partitionNo << endl);

                ProcessImageToMemory(pid, quantumLength);
                StartMain(arguments);
                systemCall = false;
                instructionPointer = "0d_?";
            }
            else if (action == "EXECUTE_NEW_PROCESS_IN_USED_PARTITION")
            {
                DBO("Executing new process [" << pid << "]    In used partition: " << partitionNo << endl);

                // Get the PID of the process using the partition and move it to the backing store
                int prevPid = GetKernelVariableIntegerValue("PID");
                DBO("    Previous PID in partition: " << prevPid << endl);
                ProcessImageToFile(prevPid, "backingstore");

                ProcessImageToMemory(pid, quantumLength);
                StartMain(arguments);
                systemCall = false;
                instructionPointer = "0d_?";
            }
            else if (action == "CONTINUE_EXECUTING_PROCESS_IN_SAME_PARTITION")
            {
                DBO("Continuing to execute process [" << pid << "]    In same partition: " << partitionNo << endl);

                SetKernelVariableValue("RUNNING_TIME", itos(0));
                SetKernelVariableValue("QUANTUM_LENGTH", itos(quantumLength));
                SetKernelVariableValue("QUANTUM_TIME_REMAINING", itos(quantumLength));
                systemCall = false;
                instructionPointer = "0d_?";
            }
            else if (action == "CONTINUE_EXECUTING_PROCESS_IN_UNUSED_PARTITION")
            {
                DBO("Continuing to execute process [" << pid << "]    In unused partition: " << partitionNo << endl);

                ProcessImageToMemory(pid, quantumLength);
                systemCall = false;
                instructionPointer = "0d_?";
            }
            else if (action == "CONTINUE_EXECUTING_PROCESS_IN_USED_PARTITION")
            {
                DBO("Continuing to execute process [" << pid << "]    In used partition: " << partitionNo << endl);

                // Get the PID of the process using the partition and move it to the backing store
                int prevPid = GetKernelVariableIntegerValue("PID");
                DBO("    Previous PID in partition: " << prevPid << endl);
                ProcessImageToFile(prevPid, "backingstore");

                ProcessImageToMemory(pid, quantumLength);
                systemCall = false;
                instructionPointer = "0d_?";
            }
            else if (action == "PAUSE_ANY_EXECUTING_PROCESS")
            {
                if (instructionPointer != "0d_null")
                {
                    pid = GetKernelVariableIntegerValue("PID");
                    runningTime = GetKernelVariableIntegerValue("RUNNING_TIME");
                    endTime = GetClockTicks();
                    ProcessImageToFile(pid, "backingstore");
                    SendMessageToHALos("EXECUTING_PROCESS_PAUSED", itos(pid), "", "", "", "", itos(endTime), itos(runningTime));
                    instructionPointer = "0d_null";
                }
                else
                {
                    SendMessageToHALos("NO_EXECUTING_PROCESS", "0", "", "", "", "", "", "");
                }
            }
            else if (action == "SHUTDOWN_OR_RESTART")
            {
                ShutdownOrRestart(command);
            }
            else
            {
                cout << "<" << __FUNCTION__ << ":" << __LINE__ << ">    ERROR: Illegal action" << endl;
                exit(1);
            }

            DBF;
        }
        UnblockSignals("HAL9000");

        if (instructionPointer != "0d_null")
        {
            if (ExecuteOneInstruction(buffer, result, systemCall))
            {
                if (systemCall)
                {
                    runningTime = GetKernelVariableIntegerValue("RUNNING_TIME");
                    endTime = GetClockTicks();
                    //
                    //ProcessImageToFile (pid, "backingstore");
                    //
                    SendMessageToHALos("SYSTEM_CALL", HALosMessage.parameter1, itos(pid), HALosMessage.parameter3, HALosMessage.parameter4, HALosMessage.parameter5, itos(endTime), itos(runningTime));
                    instructionPointer = "0d_null";
                }
            }
            else
            {
                EndMain();
                returnValue = GetKernelVariableStringValue("RETURN_VALUE");
                runningTime = GetKernelVariableIntegerValue("RUNNING_TIME");
                endTime = GetClockTicks();
                SendMessageToHALos("INTERRUPT", "PROCESS_DONE", itos(pid), returnValue, "", "", itos(endTime), itos(runningTime));
                instructionPointer = "0d_null";
            }
        }
    } while (1);

    return;
}

void StartMain(string arguments[])
{
    int functionCallValuesStackStartAddress;
    int topFunctionCallValuesStackAddress;
    int segmentSize;
    memoryCell contents;
    int i;
    int programTextEndAddress;

    functionCallValuesStackStartAddress = GetMemorySegmentBoundary("FUNCTION_CALL_VALUES_STACK_START_ADDRESS", segmentSize);
    topFunctionCallValuesStackAddress = GetKernelVariableIntegerValue("TOP_FUNCTION_CALL_VALUES_STACK_ADDRESS");
    ram.SetP(topFunctionCallValuesStackAddress);
    if (ram.GetP() == functionCallValuesStackStartAddress - segmentSize)
    {
        cout << "HAL9000: function call values stack segmentation violation" << endl;
        CoreDump();
        exit(1);
    }

    for (i = 0; i < MAX_COMMAND_LINE_ARGUMENTS; i++)
    {
        if (arguments[i].length() == 0)
        {
            break;
        }
        contents.value = arguments[i];
        if (IsInteger(contents.value))
        {
            contents.symbol = "integer";
        }
        else if (IsFloat(contents.value))
        {
            contents.symbol = "float";
        }
        else
        {
            contents.symbol = "string";
        }
        topFunctionCallValuesStackAddress = GetKernelVariableIntegerValue("TOP_FUNCTION_CALL_VALUES_STACK_ADDRESS");
        ram.SetP(topFunctionCallValuesStackAddress);
        ram.Push(contents.symbol, contents.value, partitionNo);
        topFunctionCallValuesStackAddress = ram.GetP();
        SetKernelVariableValue("TOP_FUNCTION_CALL_VALUES_STACK_ADDRESS", itos(topFunctionCallValuesStackAddress));
    }

    programTextEndAddress = GetKernelVariableIntegerValue("END_ADDRESS");
    contents.symbol = "call";
    contents.value = "main";
    Call(programTextEndAddress, contents);

    return;
}

bool ExecuteOneInstruction(string buffer, string result, bool &systemCall)
{
    int instructionPointer;
    string restartInstructionStatusFlag;
    int quantumTimeRemaining;
    memoryCell contents;
    bool executableInstruction;
    int clockTicks;
    int runningTime;

    clockTicks = GetClockTicks();
    clockTicks++;
    SetClockTicks(clockTicks);

    instructionPointer = GetKernelVariableIntegerValue("INSTRUCTION_POINTER");

    restartInstructionStatusFlag = GetKernelVariableStringValue("RESTART_INSTRUCTION_STATUS_FLAG");
    if (restartInstructionStatusFlag == "false")
    {
        instructionPointer++;
        SetKernelVariableValue("INSTRUCTION_POINTER", itos(instructionPointer));
    }

    quantumTimeRemaining = GetKernelVariableIntegerValue("QUANTUM_TIME_REMAINING");
    SetKernelVariableValue("QUANTUM_TIME_REMAINING", itos(quantumTimeRemaining - 1));

    // Increment running time
    runningTime = GetKernelVariableIntegerValue("RUNNING_TIME");
    SetKernelVariableValue("RUNNING_TIME", itos(runningTime + 1));

    if (instructionPointer <= GetKernelVariableIntegerValue("END_ADDRESS"))
    {
        ram.SetP(instructionPointer);
        contents = FetchInstruction();
        executableInstruction = DecodeInstruction(contents);
        if (executableInstruction)
        {
            systemCall = ExecuteInstruction(instructionPointer, contents, restartInstructionStatusFlag, buffer, result);
            if (systemCall)
            {
                if (restartInstructionStatusFlag == "false")
                {
                    SetKernelVariableValue("RESTART_INSTRUCTION_STATUS_FLAG", "true");
                }
                else
                {
                    SetKernelVariableValue("RESTART_INSTRUCTION_STATUS_FLAG", "false");
                    systemCall = false;
                }
            }
        }
    }
    else
    {
        return false;
    }

    return true;
}

memoryCell FetchInstruction()
{
    return (ram.Read(partitionNo));
}

bool DecodeInstruction(memoryCell contents)
{
    if (contents.symbol == "comment" || contents.symbol == "call" || contents.symbol == "return" || contents.symbol == "push" ||
        contents.symbol == "pop" || contents.symbol == "set" || contents.symbol == "compare" || contents.symbol == "add" ||
        contents.symbol == "subtract" || contents.symbol == "multiply" || contents.symbol == "divide" || contents.symbol == "modulo" ||
        contents.symbol == "join" || contents.symbol == "variable" || contents.symbol == "constant" || contents.symbol == "reference" ||
        contents.symbol == "jump" || contents.symbol == "jumpless" || contents.symbol == "jumpequal" || contents.symbol == "jumpgreater" ||
        contents.symbol == "file" || contents.symbol == "open" || contents.symbol == "read" || contents.symbol == "write" ||
        contents.symbol == "newline" || contents.symbol == "close" || contents.symbol == "clone" || contents.symbol == "run" ||
        contents.symbol == "pid" || contents.symbol == "parentpid" || contents.symbol == "coresnapshot")
    {
        return true;
    }
    else if (GetLocalSymbolAddress(contents.symbol) != -1)
    {
        return true;
    }

    return false;
}

bool ExecuteInstruction(int currentProgramTextAddress, memoryCell contents1, string restartInstructionStatusFlag, string buffer, string result)
{
    memoryCell contents2;
    int address;
    size_t foundArrayBracket;
    static string arraySize = "";
    size_t arraySizeStartPosition;
    size_t arraySizeEndPosition;

    if (contents1.symbol == "comment")
    {
        Comment();
        return false;
    }
    else if (contents1.symbol == "call")
    {
        Call(currentProgramTextAddress, contents1);
        return false;
    }
    else if (contents1.symbol == "return")
    {
        Return(contents1);
        return false;
    }
    else if (contents1.symbol == "push")
    {
        Push(contents1);
        return false;
    }
    else if (contents1.symbol == "pop")
    {
        Pop();
        return false;
    }
    else if (contents1.symbol == "set")
    {
        Set(contents1);
        return false;
    }
    else if (contents1.symbol == "compare")
    {
        Compare(contents1.value);
        return false;
    }
    else if (contents1.symbol == "jump" || contents1.symbol == "jumpless" || contents1.symbol == "jumpequal" || contents1.symbol == "jumpgreater")
    {
        Jump(contents1);
        return false;
    }
    else if (contents1.symbol == "add" || contents1.symbol == "subtract" || contents1.symbol == "multiply" || contents1.symbol == "divide" || contents1.symbol == "modulo")
    {
        DoTheMath(contents1.symbol);
        return false;
    }
    else if (contents1.symbol == "join")
    {
        Join();
        return false;
    }
    else if (contents1.symbol == "open")
    {
        Open(contents1.value, restartInstructionStatusFlag, result);
        return true;
    }
    else if (contents1.symbol == "read")
    {
        Read(contents1.value, restartInstructionStatusFlag, buffer, result);
        return true;
    }
    else if (contents1.symbol == "write")
    {
        Write(contents1.value, restartInstructionStatusFlag, result);
        return true;
    }
    else if (contents1.symbol == "newline")
    {
        Newline(contents1.value, restartInstructionStatusFlag, result);
        return true;
    }
    else if (contents1.symbol == "close")
    {
        Close(contents1.value, restartInstructionStatusFlag, result);
        return true;
    }
    else if (contents1.symbol == "reference" || contents1.symbol == "file")
    {
        AllocateLocalSymbol(contents1);
        return false;
    }
    else if (contents1.symbol == "variable" || contents1.symbol == "constant")
    {
        foundArrayBracket = contents1.value.find("<");
        if (foundArrayBracket != string::npos)
        {
            arraySizeStartPosition = foundArrayBracket + 1;
            arraySizeEndPosition = contents1.value.find(">") - 1;
            arraySize = contents1.value.substr(arraySizeStartPosition, arraySizeEndPosition - arraySizeStartPosition + 1);
            contents1.value = contents1.value.substr(0, foundArrayBracket);
        }
        AllocateLocalSymbol(contents1);
        return false;
    }
    else if (contents1.symbol == "clone")
    {
        Clone(restartInstructionStatusFlag, buffer, result);
        return true;
    }
    else if (contents1.symbol == "run")
    {
        Run(contents1.value, restartInstructionStatusFlag);
        return true;
    }
    else if (contents1.symbol == "pid")
    {
        GetPid("PID");
        return false;
    }
    else if (contents1.symbol == "parentpid")
    {
        GetPid("PARENT_PID");
        return false;
    }
    else if (contents1.symbol == "coresnapshot")
    {
        CoreSnapShot();
        return false;
    }
    else if (GetLocalSymbolAddress(contents1.symbol) != -1 && (contents1.value == "integer" || contents1.value == "float" || contents1.value == "string" || contents1.value == "address" || contents1.value == "input" || contents1.value == "output"))
    {
        AssignTypeToLocalSymbol(contents1);
        if (arraySize.length() > 0)
        {
            if (IsInteger(arraySize))
            {
                contents1.value = arraySize;
                AllocateLocalArray(contents1);
            }
            else
            {
                address = GetLocalSymbolAddress(arraySize);
                if (address != -1)
                {
                    ram.SetP(address);
                    contents2 = ram.Read(partitionNo);
                    ram.SetP(atoi(contents2.value.c_str()));
                    contents2 = ram.Read(partitionNo);
                    contents1.value = contents2.value;
                    AllocateLocalArray(contents1);
                }
                else
                {
                    address = GetGlobalSymbolAddress(arraySize);
                    if (address != -1)
                    {
                        ram.SetP(address);
                        contents2 = ram.Read(partitionNo);
                        ram.SetP(atoi(contents2.value.c_str()));
                        contents2 = ram.Read(partitionNo);
                        contents1.value = contents2.value;
                        AllocateLocalArray(contents1);
                    }
                    else
                    {
                        cout << "HAL9000: " << arraySize << " is an undeclared symbol" << endl;
                        CoreDump();
                        exit(1);
                    }
                }
            }
            arraySize = "";
        }
        return false;
    }

    return false;
}

void EndMain()
{
    memoryCell contents;

    ram.SetP(GetKernelVariableIntegerValue("TOP_FUNCTION_CALL_VALUES_STACK_ADDRESS"));
    contents = ram.Pop(partitionNo);
    SetKernelVariableValue("TOP_FUNCTION_CALL_VALUES_STACK_ADDRESS", itos(ram.GetP()));
    SetKernelVariableValue("RETURN_VALUE", contents.value);

    return;
}

// The "Communication Media"

string GetMessageFromHALos(int &pid, string &command, string arguments[], string &buffer, string &result, int &quantumLength)
{
    static int seqNo = 0;
    static string fileNamePrefix = "HALosToHAL9000_";
    static string fileName;
    ifstream halOsMessageFile;
    string action;
    int i;

    if (seqNo > 0)
    {
        remove(fileName.c_str());
    }

    seqNo++;
    fileName = fileNamePrefix + itos(seqNo);
    halOsMessageFile.open(fileName.c_str());
    if (!halOsMessageFile)
    {
        cout << "HAL9000: connection to HALos failed" << endl;
        CoreDump();
        exit(1);
    }

    getline(halOsMessageFile, action);
    halOsMessageFile >> pid;
    halOsMessageFile.ignore(256, '\n');
    getline(halOsMessageFile, command);
    for (i = 0; i < MAX_COMMAND_LINE_ARGUMENTS; i++)
    {
        getline(halOsMessageFile, arguments[i]);
    }
    getline(halOsMessageFile, buffer);
    getline(halOsMessageFile, result);
    halOsMessageFile >> quantumLength;
    halOsMessageFile >> partitionNo;

#ifdef HAL9000_MESSAGE_TRACE_ON
    {
        cout << endl;
        cout << "HAL9000: message received from HALos for pid = " << pid << endl;
        cout << "    action = " << action << endl;
        cout << "    pid = " << pid << endl;
        cout << "    command = " << command << endl;
        for (i = 0; i < MAX_COMMAND_LINE_ARGUMENTS; i++)
        {
            cout << "    arguments [" << i << "] = " << arguments[i] << endl;
        }
        cout << "    buffer = " << buffer << endl;
        cout << "    result = " << result << endl;
        cout << "    quantumLength = " << quantumLength << endl;
        cout << "    partitionNo = " << partitionNo << endl;
    }
#endif

    if (!halOsMessageFile)
    {
        cout << "HAL9000: message from HALos corrupted" << endl;
        exit(1);
    }

    halOsMessageFile.close();

    if (seqNo == INT_MAX)
    {
        seqNo = 1;
    }

    return action;
}

void SendMessageToHALos(string type, string parameter1, string parameter2, string parameter3, string parameter4, string parameter5, string parameter6, string parameter7)
{
    static int seqNo = 0;
    static string fileNamePrefix = "HAL9000ToHALos_";
    string fileName;
    ofstream hal9000MessageFile;
    union sigval dummyValue;

    seqNo++;
    fileName = fileNamePrefix + itos(seqNo);
    hal9000MessageFile.open(fileName.c_str());
    if (!hal9000MessageFile)
    {
        cout << "HAL9000: unable to initialize HALos message buffer" << endl;
        exit(1);
    }

    hal9000MessageFile << type << endl;
    hal9000MessageFile << parameter1 << endl;
    hal9000MessageFile << parameter2 << endl;
    hal9000MessageFile << parameter3 << endl;
    hal9000MessageFile << parameter4 << endl;
    hal9000MessageFile << parameter5 << endl;
    hal9000MessageFile << parameter6 << endl;
    hal9000MessageFile << parameter7 << endl;

#ifdef HAL9000_MESSAGE_TRACE_ON
    {
        cout << endl;
        if (type == "EXECUTING_PROCESS_PAUSED" || type == "NO_EXECUTING_PROCESS")
        {
            cout << "HAL9000: message sent to HALos for pid = " << parameter1 << endl;
        }
        else // (type == "INTERRUPT" || type == "SYSTEM_CALL")
        {
            cout << "HAL9000: message sent to HALos for pid = " << parameter2 << endl;
        }
        cout << "    type = " << type << endl;
        cout << "    parameter1 = " << parameter1 << endl;
        cout << "    parameter2 = " << parameter2 << endl;
        cout << "    parameter3 = " << parameter3 << endl;
        cout << "    parameter4 = " << parameter4 << endl;
        cout << "    parameter5 = " << parameter5 << endl;
        cout << "    parameter6 = " << parameter6 << endl;
        cout << "    parameter7 = " << parameter7 << endl;
    }
#endif

    hal9000MessageFile.close();

    if (sigqueue(HALosPid, SIGRTMIN, dummyValue) == -1)
    {
        cout << "HAL9000: message ready signal not sent to HALos" << endl;
        exit(1);
    }

    if (seqNo == INT_MAX)
    {
        seqNo = 1;
    }

    return;
}

// "Built-in" Instructions

void Comment()
{
    return;
}

void Call(int currentProgramTextAddress, memoryCell contents)
{
    int functionCallStackStartAddress;
    int topFunctionCallStackAddress;
    int functionCallValuesStackStartAddress;
    int topFunctionCallValuesStackAddress;
    int segmentSize;
    int address;
    string functionName = contents.value;

    functionCallValuesStackStartAddress = GetMemorySegmentBoundary("FUNCTION_CALL_VALUES_STACK_START_ADDRESS", segmentSize);
    topFunctionCallValuesStackAddress = GetKernelVariableIntegerValue("TOP_FUNCTION_CALL_VALUES_STACK_ADDRESS");
    ram.SetP(topFunctionCallValuesStackAddress);
    if (ram.GetP() == functionCallValuesStackStartAddress - segmentSize)
    {
        cout << "HAL9000: function call values stack segmentation violation" << endl;
        return;
    }
    ram.Push("return_address", itos(currentProgramTextAddress), partitionNo);
    topFunctionCallValuesStackAddress = ram.GetP();
    SetKernelVariableValue("TOP_FUNCTION_CALL_VALUES_STACK_ADDRESS", itos(topFunctionCallValuesStackAddress));
    SetKernelVariableValue("NEXT_ARGUMENT_ADDRESS", itos(topFunctionCallValuesStackAddress + 1));

    functionCallStackStartAddress = GetMemorySegmentBoundary("FUNCTION_CALL_STACK_START_ADDRESS", segmentSize);
    topFunctionCallStackAddress = GetKernelVariableIntegerValue("TOP_FUNCTION_CALL_STACK_ADDRESS");
    ram.SetP(topFunctionCallStackAddress);
    if (ram.GetP() == functionCallStackStartAddress - segmentSize)
    {
        cout << "HAL9000: function call stack segmentation violation" << endl;
        CoreDump();
        exit(1);
    }
    ram.Push("call_" + functionName, itos(topFunctionCallValuesStackAddress), partitionNo);
    topFunctionCallStackAddress = ram.GetP();
    SetKernelVariableValue("TOP_FUNCTION_CALL_STACK_ADDRESS", itos(topFunctionCallStackAddress));

    address = GetGlobalSymbolAddress(functionName);
    if (address != -1)
    {
        ram.SetP(address);
        contents = ram.Read(partitionNo);
        ram.SetP(atoi(contents.value.c_str()));
        contents = ram.Read(partitionNo);
        SetKernelVariableValue("INSTRUCTION_POINTER", contents.value);
    }
    else
    {
        cout << "HAL9000: function " << functionName << " not defined" << endl;
        CoreDump();
        exit(1);
    }

    return;
}

void Return(memoryCell contents1)
{
    int functionCallValuesStackStartAddress;
    int topFunctionCallValuesStackAddress;
    int segmentSize;
    memoryCell contents2;

    functionCallValuesStackStartAddress = GetMemorySegmentBoundary("FUNCTION_CALL_VALUES_STACK_START_ADDRESS", segmentSize);
    topFunctionCallValuesStackAddress = GetKernelVariableIntegerValue("TOP_FUNCTION_CALL_VALUES_STACK_ADDRESS");
    ram.SetP(topFunctionCallValuesStackAddress);
    if (ram.GetP() == functionCallValuesStackStartAddress - segmentSize)
    {
        cout << "HAL9000: function call values stack segmentation violation" << endl;
        CoreDump();
        exit(1);
    }

    if (contents1.value.length() == 0)
    {
        contents2.symbol = "";
        contents2.value = "";
    }
    else
    {
        contents2 = DetermineMemoryCellContentsForReturnAndPushCommands(contents1);
    }

    ram.SetP(GetKernelVariableIntegerValue("TOP_FUNCTION_CALL_STACK_ADDRESS"));
    contents1 = ram.Read(partitionNo);
    while (1)
    {
        if (contents1.symbol.substr(0, 5) != "call_")
        {
            contents1 = ram.Pop(partitionNo);
            SetKernelVariableValue("TOP_FUNCTION_CALL_STACK_ADDRESS", itos(ram.GetP()));
            ram.SetP(GetKernelVariableIntegerValue("TOP_FUNCTION_CALL_STACK_ADDRESS"));
            contents1 = ram.Read(partitionNo);
        }
        else
        {
            contents1 = ram.Pop(partitionNo);
            SetKernelVariableValue("TOP_FUNCTION_CALL_STACK_ADDRESS", itos(ram.GetP()));
            break;
        }
    }

    ram.SetP(GetKernelVariableIntegerValue("TOP_FUNCTION_CALL_VALUES_STACK_ADDRESS"));
    contents1 = ram.Read(partitionNo);
    while (1)
    {
        if (contents1.symbol != "return_address")
        {
            contents1 = ram.Pop(partitionNo);
            SetKernelVariableValue("TOP_FUNCTION_CALL_VALUES_STACK_ADDRESS", itos(ram.GetP()));
            ram.SetP(GetKernelVariableIntegerValue("TOP_FUNCTION_CALL_VALUES_STACK_ADDRESS"));
            contents1 = ram.Read(partitionNo);
        }
        else
        {
            contents1 = ram.Pop(partitionNo);
            SetKernelVariableValue("TOP_FUNCTION_CALL_VALUES_STACK_ADDRESS", itos(ram.GetP()));
            break;
        }
    }
    SetKernelVariableValue("INSTRUCTION_POINTER", contents1.value);

    ram.SetP(GetKernelVariableIntegerValue("TOP_FUNCTION_CALL_VALUES_STACK_ADDRESS"));
    ram.Push(contents2.symbol, contents2.value, partitionNo);
    SetKernelVariableValue("TOP_FUNCTION_CALL_VALUES_STACK_ADDRESS", itos(ram.GetP()));

    return;
}

void Push(memoryCell contents1)
{
    int functionCallValuesStackStartAddress;
    int topFunctionCallValuesStackAddress;
    int segmentSize;
    memoryCell contents2;

    functionCallValuesStackStartAddress = GetMemorySegmentBoundary("FUNCTION_CALL_VALUES_STACK_START_ADDRESS", segmentSize);
    topFunctionCallValuesStackAddress = GetKernelVariableIntegerValue("TOP_FUNCTION_CALL_VALUES_STACK_ADDRESS");
    ram.SetP(topFunctionCallValuesStackAddress);
    if (ram.GetP() == functionCallValuesStackStartAddress - segmentSize)
    {
        cout << "HAL9000: function call values stack segmentation violation" << endl;
        CoreDump();
        exit(1);
    }

    // push :argument
    if (contents1.value == "argument")
    {
        ram.SetP(GetKernelVariableIntegerValue("NEXT_ARGUMENT_ADDRESS"));
        contents1 = ram.Read(partitionNo);
        contents2.symbol = GetDataType(contents1.symbol);
        contents2.value = contents1.value;
        SetKernelVariableValue("NEXT_ARGUMENT_ADDRESS", itos(ram.GetP() + 1));
    }
    else
    {
        contents2 = DetermineMemoryCellContentsForReturnAndPushCommands(contents1);
    }

    ram.SetP(GetKernelVariableIntegerValue("TOP_FUNCTION_CALL_VALUES_STACK_ADDRESS"));
    ram.Push(contents2.symbol, contents2.value, partitionNo);
    SetKernelVariableValue("TOP_FUNCTION_CALL_VALUES_STACK_ADDRESS", itos(ram.GetP()));

    return;
}

memoryCell DetermineMemoryCellContentsForReturnAndPushCommands(memoryCell contents1)
{
    memoryCell contents2;
    int address;
    int symbolAddress;
    int indexAddress;
    bool isLocalSymbol = true;
    size_t foundArrayBracket;
    size_t foundArrayAddress;
    size_t foundFileAddress;
    string index;
    size_t indexStartPosition;
    size_t indexEndPosition;

    // push/return string
    if (contents1.value[0] == '\'')
    {
        contents2.symbol = "string";
        if (contents1.value.length() > 1)
        {
            contents2.value = contents1.value.substr(1, contents2.value.length() - 1);
        }
        else
        {
            contents2.value = "";
        }
    }
    // push/return :@a or push/return :@a<?> (? is literal integer constant, variable name, or constant name)
    else if (contents1.value[0] == '@')
    {
        contents1.value = contents1.value.substr(1, contents1.value.length() - 1);
        foundArrayBracket = contents1.value.find("<");
        // return :@a
        if (foundArrayBracket == string::npos)
        {
            symbolAddress = GetLocalSymbolAddress(contents1.value);
            if (symbolAddress == -1)
            {
                symbolAddress = GetGlobalSymbolAddress(contents1.value);
                if (symbolAddress == -1)
                {
                    cout << "HAL9000: " << contents1.value << " not defined" << endl;
                    CoreDump();
                    exit(1);
                }
            }
            ram.SetP(symbolAddress);
            contents1 = ram.Read(partitionNo);
            address = atoi(contents1.value.c_str());
            ram.SetP(atoi(contents1.value.c_str()));
            contents1 = ram.Read(partitionNo);
            contents2.symbol = contents1.symbol + "_address";
            contents2.value = itos(address);
        }
        // push/return :@a<?> (? is a literal integer constant, variable name, or constant name)
        else
        {
            indexStartPosition = foundArrayBracket + 1;
            indexEndPosition = contents1.value.find(">") - 1;
            index = contents1.value.substr(indexStartPosition, indexEndPosition - indexStartPosition + 1);
            contents1.value = contents1.value.substr(0, indexStartPosition - 1);
            symbolAddress = GetLocalSymbolAddress(contents1.value);
            if (symbolAddress == -1)
            {
                isLocalSymbol = false;
                symbolAddress = GetGlobalSymbolAddress(contents1.value);
                if (symbolAddress == -1)
                {
                    cout << "HAL9000: " << contents1.value << " not defined" << endl;
                    CoreDump();
                    exit(1);
                }
            }
            ram.SetP(symbolAddress);
            contents1 = ram.Read(partitionNo);
            address = atoi(contents1.value.c_str());
            ram.SetP(atoi(contents1.value.c_str()));
            contents1 = ram.Read(partitionNo);
            foundArrayAddress = contents1.symbol.find("array");
            contents2.symbol = contents1.symbol.substr(0, foundArrayAddress - 1) + "_address";
            // push/return :@a<9> (9 is any literal integer constant)
            if (IsInteger(index))
            {
                if (isLocalSymbol)
                {
                    address = address - atoi(index.c_str());
                }
                else
                {
                    address = address + atoi(index.c_str());
                }
                contents2.value = itos(address);
            }
            // push/return :@a<i> (i is a variable name or constant name)
            else
            {
                indexAddress = GetLocalSymbolAddress(index);
                if (indexAddress == -1)
                {
                    indexAddress = GetGlobalSymbolAddress(index);
                    if (indexAddress == -1)
                    {
                        cout << "HAL9000: " << index << " not defined" << endl;
                        CoreDump();
                        exit(1);
                    }
                }
                ram.SetP(indexAddress);
                contents1 = ram.Read(partitionNo);
                ram.SetP(atoi(contents1.value.c_str()));
                contents1 = ram.Read(partitionNo);
                if (isLocalSymbol)
                {
                    address = address - atoi(contents1.value.c_str());
                }
                else
                {
                    address = address + atoi(contents1.value.c_str());
                }
                contents2.value = itos(address);
            }
        }
    }
    else
    {
        foundArrayBracket = contents1.value.find("<");
        // push/return :a<?> (? is a literal integer constant, variable name, or constant name)
        if (foundArrayBracket != string::npos)
        {
            indexStartPosition = foundArrayBracket + 1;
            indexEndPosition = contents1.value.find(">") - 1;
            index = contents1.value.substr(indexStartPosition, indexEndPosition - indexStartPosition + 1);
            contents1.value = contents1.value.substr(0, indexStartPosition - 1);
            symbolAddress = GetLocalSymbolAddress(contents1.value);
            if (symbolAddress == -1)
            {
                isLocalSymbol = false;
                symbolAddress = GetGlobalSymbolAddress(contents1.value);
                if (symbolAddress == -1)
                {
                    cout << "HAL9000: " << contents1.value << " not defined" << endl;
                    CoreDump();
                    exit(1);
                }
            }
            ram.SetP(symbolAddress);
            contents1 = ram.Read(partitionNo);
            address = atoi(contents1.value.c_str());
            ram.SetP(atoi(contents1.value.c_str()));
            contents1 = ram.Read(partitionNo);
            contents2.symbol = GetDataType(contents1.symbol);
            // push/return :a<9> (9 is any literal integer constant)
            if (IsInteger(index))
            {
                if (isLocalSymbol)
                {
                    ram.SetP(address - atoi(index.c_str()));
                }
                else
                {
                    ram.SetP(address + atoi(index.c_str()));
                }
            }
            // push/return :a<i> (i is a variable name or constant name)
            else
            {
                indexAddress = GetLocalSymbolAddress(index);
                if (indexAddress == -1)
                {
                    indexAddress = GetGlobalSymbolAddress(index);
                    if (indexAddress == -1)
                    {
                        cout << "HAL9000: " << index << " not defined" << endl;
                        CoreDump();
                        exit(1);
                    }
                }
                ram.SetP(indexAddress);
                contents1 = ram.Read(partitionNo);
                ram.SetP(atoi(contents1.value.c_str()));
                contents1 = ram.Read(partitionNo);
                if (isLocalSymbol)
                {
                    ram.SetP(address - atoi(contents1.value.c_str()));
                }
                else
                {
                    ram.SetP(address + atoi(contents1.value.c_str()));
                }
            }
            contents1 = ram.Read(partitionNo);
            contents2.value = contents1.value;
        }
        // push/return :a or push/return :9 or push/return :9.9 or push/return :s
        else
        {
            symbolAddress = GetLocalSymbolAddress(contents1.value);
            if (symbolAddress == -1)
            {
                isLocalSymbol = false;
                symbolAddress = GetGlobalSymbolAddress(contents1.value);
            }
            if (symbolAddress != -1)
            {
                ram.SetP(symbolAddress);
                contents1 = ram.Read(partitionNo);
                ram.SetP(atoi(contents1.value.c_str()));
                contents1 = ram.Read(partitionNo);
                foundArrayAddress = contents1.symbol.find("array");
                // push/return :a (where a is not an array name)
                if (foundArrayAddress == string::npos)
                {
                    foundFileAddress = contents1.symbol.find("file");
                    if (foundFileAddress == string::npos)
                    {
                        contents2.symbol = GetDataType(contents1.symbol);
                        contents2.value = contents1.value;
                    }
                    // push/return :a (where a is a file)
                    else
                    {
                        contents2.symbol = GetDataType(contents1.symbol);
                        if (isLocalSymbol)
                        {
                            contents2.symbol = "local_" + contents2.symbol + "_address";
                        }
                        else
                        {
                            contents2.symbol = "global_" + contents2.symbol + "_address";
                        }
                        contents2.value = itos(ram.GetP());
                    }
                }
                // push/return :a (where a is an array name)
                else
                {
                    foundArrayAddress = contents1.symbol.find("0");
                    contents2.symbol = contents1.symbol.substr(0, foundArrayAddress - 1);
                    if (isLocalSymbol)
                    {
                        contents2.symbol = "local_" + contents2.symbol + "_address";
                    }
                    else
                    {
                        contents2.symbol = "global_" + contents2.symbol + "_address";
                    }
                    contents2.value = itos(ram.GetP());
                }
            }
            // push/return :9 (9 is any integer)
            else if (IsInteger(contents1.value))
            {
                contents2.symbol = "integer";
                contents2.value = contents1.value;
            }
            // push/return :9.9 (9.9 is any float)
            else if (IsFloat(contents1.value))
            {
                contents2.symbol = "float";
                contents2.value = contents1.value;
            }
            // push/return :s (s is any string)
            else
            {
                contents2.symbol = "string";
                contents2.value = contents1.value;
            }
        }
    }

    return contents2;
}

void Set(memoryCell contents)
{
    string symbol;
    string value;
    size_t foundArrayBracket;
    string index;
    size_t indexStartPosition;
    size_t indexEndPosition;
    int addressOffset = 0;
    int address;
    string dataTypeCategory;

    symbol = contents.value;

    ram.SetP(GetKernelVariableIntegerValue("TOP_FUNCTION_CALL_VALUES_STACK_ADDRESS"));
    contents = ram.Pop(partitionNo);
    value = contents.value;
    SetKernelVariableValue("TOP_FUNCTION_CALL_VALUES_STACK_ADDRESS", itos(ram.GetP()));

    if (symbol[0] == '@')
    {
        symbol = symbol.substr(1, symbol.length() - 1);
    }
    foundArrayBracket = symbol.find("<");
    if (foundArrayBracket != string::npos)
    {
        indexStartPosition = foundArrayBracket + 1;
        indexEndPosition = symbol.find(">") - 1;
        index = symbol.substr(indexStartPosition, indexEndPosition - indexStartPosition + 1);
        if (IsInteger(index))
        {
            symbol = symbol.substr(0, indexStartPosition - 1);
            addressOffset = atoi(index.c_str());
        }
        else
        {
            address = GetLocalSymbolAddress(index);
            if (address != -1)
            {
                ram.SetP(address);
                contents = ram.Read(partitionNo);
                symbol = symbol.substr(0, indexStartPosition - 1);
                ram.SetP(atoi(contents.value.c_str()));
                contents = ram.Read(partitionNo);
                addressOffset = atoi(contents.value.c_str());
            }
            else
            {
                address = GetGlobalSymbolAddress(index);
                if (address != -1)
                {
                    ram.SetP(address);
                    contents = ram.Read(partitionNo);
                    symbol = symbol.substr(0, indexStartPosition - 1);
                    ram.SetP(atoi(contents.value.c_str()));
                    contents = ram.Read(partitionNo);
                    addressOffset = atoi(contents.value.c_str());
                }
                else
                {
                    cout << "HAL9000: " << index << " is an undeclared symbol" << endl;
                    CoreDump();
                    exit(1);
                }
            }
        }
    }

    address = GetLocalSymbolAddress(symbol);
    if (address != -1)
    {
        ram.SetP(address);
        contents = ram.Read(partitionNo);
        ram.SetP(atoi(contents.value.c_str()) - addressOffset);
        contents = ram.Read(partitionNo);
        if (contents.symbol == "input_file" || contents.symbol == "output_file")
        {
            address = ram.GetP();
            ram.SetP(address - 1);
            contents = ram.Read(partitionNo);
        }
        else
        {
            dataTypeCategory = GetDataTypeCategory(contents.symbol);
            if (dataTypeCategory == "constant" && contents.value != "?#@NULL_VALUE@#?")
            {
                cout << "HAL9000: local constant " << symbol << " has already been initialized" << endl;
                CoreDump();
                exit(1);
            }
        }
    }
    else
    {
        address = GetGlobalSymbolAddress(symbol);
        if (address != -1)
        {
            ram.SetP(address);
            contents = ram.Read(partitionNo);
            ram.SetP(atoi(contents.value.c_str()) + addressOffset);
            contents = ram.Read(partitionNo);
            if (contents.symbol == "input_file" || contents.symbol == "output_file")
            {
                address = ram.GetP();
                ram.SetP(address + 1);
                contents = ram.Read(partitionNo);
            }
            else
            {
                dataTypeCategory = GetDataTypeCategory(contents.symbol);
                if (dataTypeCategory == "constant" && contents.value != "?#@NULL_VALUE@#?")
                {
                    cout << "HAL9000: global constant " << symbol << " has already been initialized" << endl;
                    CoreDump();
                    exit(1);
                }
            }
        }
        else
        {
            cout << "HAL9000: " << symbol << " is an undeclared symbol" << endl;
            CoreDump();
            exit(1);
        }
    }

    ram.ReWrite("", value, partitionNo);

    return;
}

void Compare(string value)
{
    if (value.length() == 0)
    {
        CompareValues();
    }
    else if (value == "eof")
    {
        CompareEndOfFile();
    }
    else // (value == "integer" || "float" || "string" || "emptystring")
    {
        CompareDataTypes(value);
    }

    return;
}

void CompareEndOfFile()
{
    memoryCell contents;
    int comparisonStatusFlag;

    ram.SetP(GetKernelVariableIntegerValue("TOP_FUNCTION_CALL_VALUES_STACK_ADDRESS"));
    contents = ram.Pop(partitionNo);
    SetKernelVariableValue("TOP_FUNCTION_CALL_VALUES_STACK_ADDRESS", itos(ram.GetP()));

    if (contents.symbol == "eof")
    {
        comparisonStatusFlag = 0;
    }
    else
    {
        comparisonStatusFlag = -2;
        ram.SetP(GetKernelVariableIntegerValue("TOP_FUNCTION_CALL_VALUES_STACK_ADDRESS"));
        ram.Push(contents.symbol, contents.value, partitionNo);
        SetKernelVariableValue("TOP_FUNCTION_CALL_VALUES_STACK_ADDRESS", itos(ram.GetP()));
    }

    SetKernelVariableValue("COMPARISON_STATUS_FLAG", itos(comparisonStatusFlag));

    return;
}

void CompareDataTypes(string dataType)
{
    memoryCell contents;
    int comparisonStatusFlag;

    ram.SetP(GetKernelVariableIntegerValue("TOP_FUNCTION_CALL_VALUES_STACK_ADDRESS"));
    contents = ram.Pop(partitionNo);
    SetKernelVariableValue("TOP_FUNCTION_CALL_VALUES_STACK_ADDRESS", itos(ram.GetP()));

    if (dataType == "integer")
    {
        if (IsInteger(contents.value))
        {
            comparisonStatusFlag = 0;
            contents.symbol = "integer";
        }
        else
        {
            comparisonStatusFlag = -2;
        }
    }
    else if (dataType == "float")
    {
        if (IsFloat(contents.value))
        {
            comparisonStatusFlag = 0;
            contents.symbol = "float";
        }
        else
        {
            comparisonStatusFlag = -2;
        }
    }
    else if (dataType == "string")
    {
        if (contents.value.length() > 0)
        {
            comparisonStatusFlag = 0;
        }
        else
        {
            comparisonStatusFlag = -2;
        }
    }
    else if (dataType == "emptystring")
    {
        if (contents.value.length() == 0)
        {
            comparisonStatusFlag = 0;
        }
        else
        {
            comparisonStatusFlag = -2;
        }
    }

    ram.SetP(GetKernelVariableIntegerValue("TOP_FUNCTION_CALL_VALUES_STACK_ADDRESS"));
    ram.Push(contents.symbol, contents.value, partitionNo);
    SetKernelVariableValue("TOP_FUNCTION_CALL_VALUES_STACK_ADDRESS", itos(ram.GetP()));

    SetKernelVariableValue("COMPARISON_STATUS_FLAG", itos(comparisonStatusFlag));

    return;
}

void CompareValues()
{
    memoryCell contents1;
    memoryCell contents2;
    int integerValue1;
    int integerValue2;
    double floatValue1;
    double floatValue2;
    string stringValue1;
    string stringValue2;
    int comparisonStatusFlag;

    ram.SetP(GetKernelVariableIntegerValue("TOP_FUNCTION_CALL_VALUES_STACK_ADDRESS"));
    contents2 = ram.Pop(partitionNo);
    SetKernelVariableValue("TOP_FUNCTION_CALL_VALUES_STACK_ADDRESS", itos(ram.GetP()));

    ram.SetP(GetKernelVariableIntegerValue("TOP_FUNCTION_CALL_VALUES_STACK_ADDRESS"));
    contents1 = ram.Pop(partitionNo);
    SetKernelVariableValue("TOP_FUNCTION_CALL_VALUES_STACK_ADDRESS", itos(ram.GetP()));

    if (contents1.symbol == "integer")
    {
        integerValue1 = atoi(contents1.value.c_str());
        if (contents2.symbol == "integer")
        {
            integerValue2 = atoi(contents2.value.c_str());
            if (integerValue1 == integerValue2)
            {
                comparisonStatusFlag = 0;
            }
            else if (integerValue1 > integerValue2)
            {
                comparisonStatusFlag = 1;
            }
            else
            {
                comparisonStatusFlag = -1;
            }
        }
        else if (contents2.symbol == "float")
        {
            floatValue2 = atof(contents2.value.c_str());
            if (integerValue1 == floatValue2)
            {
                comparisonStatusFlag = 0;
            }
            else if (integerValue1 > floatValue2)
            {
                comparisonStatusFlag = 1;
            }
            else
            {
                comparisonStatusFlag = -1;
            }
        }
        else
        {
            cout << "HAL9000: integer and string types mismatched in compare operation" << endl;
            CoreDump();
            exit(1);
        }
    }
    else if (contents1.symbol == "float")
    {
        floatValue1 = atof(contents1.value.c_str());
        if (contents2.symbol == "integer")
        {
            integerValue2 = atoi(contents2.value.c_str());
            if (floatValue1 == integerValue2)
            {
                comparisonStatusFlag = 0;
            }
            else if (floatValue1 > integerValue2)
            {
                comparisonStatusFlag = 1;
            }
            else
            {
                comparisonStatusFlag = -1;
            }
        }
        else if (contents2.symbol == "float")
        {
            floatValue2 = atof(contents2.value.c_str());
            if (floatValue1 == floatValue2)
            {
                comparisonStatusFlag = 0;
            }
            else if (floatValue1 > floatValue2)
            {
                comparisonStatusFlag = 1;
            }
            else
            {
                comparisonStatusFlag = -1;
            }
        }
        else
        {
            cout << "HAL9000: float and string types mismatched in compare operation" << endl;
            CoreDump();
            exit(1);
        }
    }
    else // (contents1.symbol == "string")
    {
        stringValue1 = contents1.value;
        if (contents2.symbol == "integer")
        {
            cout << "HAL9000: string and integer types mismatched in compare operation" << endl;
            CoreDump();
            exit(1);
        }
        else if (contents2.symbol == "float")
        {
            cout << "HAL9000: string and float types mismatched in compare operation" << endl;
            CoreDump();
            exit(1);
        }
        else
        {
            stringValue2 = contents2.value;
            if (stringValue1 == stringValue2)
            {
                comparisonStatusFlag = 0;
            }
            else if (stringValue1 > stringValue2)
            {
                comparisonStatusFlag = 1;
            }
            else
            {
                comparisonStatusFlag = -1;
            }
        }
    }

    SetKernelVariableValue("COMPARISON_STATUS_FLAG", itos(comparisonStatusFlag));

    return;
}

void Pop()
{
    memoryCell contents;

    ram.SetP(GetKernelVariableIntegerValue("TOP_FUNCTION_CALL_VALUES_STACK_ADDRESS"));
    contents = ram.Pop(partitionNo);
    SetKernelVariableValue("TOP_FUNCTION_CALL_VALUES_STACK_ADDRESS", itos(ram.GetP()));

    return;
}

void Jump(memoryCell contents)
{
    int address;
    string labelName;
    int comparisonStatusFlag;

    labelName = contents.value;

    comparisonStatusFlag = GetKernelVariableIntegerValue("COMPARISON_STATUS_FLAG");
    if ((contents.symbol == "jump") ||
        (contents.symbol == "jumpless" && comparisonStatusFlag == -1) ||
        (contents.symbol == "jumpequal" && comparisonStatusFlag == 0) ||
        (contents.symbol == "jumpgreater" && comparisonStatusFlag == 1))
    {
        address = GetGlobalSymbolAddress(labelName);
        if (address != -1)
        {
            ram.SetP(address);
            contents = ram.Read(partitionNo);
            ram.SetP(atoi(contents.value.c_str()));
            contents = ram.Read(partitionNo);
            SetKernelVariableValue("INSTRUCTION_POINTER", contents.value);
            SetKernelVariableValue("COMPARISON_STATUS_FLAG", itos(-2));
        }
        else
        {
            cout << "HAL9000: label " << labelName << " does not exist" << endl;
            CoreDump();
            exit(1);
        }
    }

    return;
}

void DoTheMath(string operation)
{
    int functionCallValuesStackStartAddress;
    int segmentSize;
    int topAddress;
    memoryCell contents1;
    memoryCell contents2;
    int integerOperand1;
    double floatOperand1;
    int integerOperand2;
    double floatOperand2;
    int integerResult;
    double floatResult;
    size_t foundAddress;

    functionCallValuesStackStartAddress = GetMemorySegmentBoundary("FUNCTION_CALL_VALUES_STACK_START_ADDRESS", segmentSize);
    topAddress = GetKernelVariableIntegerValue("TOP_FUNCTION_CALL_VALUES_STACK_ADDRESS");
    ram.SetP(topAddress);
    if (ram.GetP() > functionCallValuesStackStartAddress)
    {
        cout << "HAL9000: function call values stack segmentation violation" << endl;
        CoreDump();
        exit(1);
    }
    contents2 = ram.Pop(partitionNo);
    topAddress = ram.GetP();
    SetKernelVariableValue("TOP_FUNCTION_CALL_VALUES_STACK_ADDRESS", itos(topAddress));
    foundAddress = contents2.symbol.find("address");
    if (foundAddress != string::npos)
    {
        ram.SetP(atoi(contents2.value.c_str()));
        contents2 = ram.Read(partitionNo);
        contents2.symbol = GetDataType(contents2.symbol);
    }

    topAddress = GetKernelVariableIntegerValue("TOP_FUNCTION_CALL_VALUES_STACK_ADDRESS");
    ram.SetP(topAddress);
    if (ram.GetP() > functionCallValuesStackStartAddress)
    {
        cout << "HAL9000: function call values stack segmentation violation" << endl;
        CoreDump();
        exit(1);
    }
    contents1 = ram.Pop(partitionNo);
    topAddress = ram.GetP();
    SetKernelVariableValue("TOP_FUNCTION_CALL_VALUES_STACK_ADDRESS", itos(topAddress));
    foundAddress = contents1.symbol.find("address");
    if (foundAddress != string::npos)
    {
        ram.SetP(atoi(contents1.value.c_str()));
        contents1 = ram.Read(partitionNo);
        contents1.symbol = GetDataType(contents1.symbol);
    }

    if (contents1.symbol == "integer")
    {
        integerOperand1 = atoi(contents1.value.c_str());
        if (contents2.symbol == "integer")
        {
            integerOperand2 = atoi(contents2.value.c_str());
            if (operation == "add")
            {
                integerResult = integerOperand1 + integerOperand2;
            }
            else if (operation == "subtract")
            {
                integerResult = integerOperand1 - integerOperand2;
            }
            else if (operation == "multiply")
            {
                integerResult = integerOperand1 * integerOperand2;
            }
            else if (operation == "divide")
            {
                if (integerOperand2 == 0)
                {
                    cout << "HAL9000: division by zero" << endl;
                    CoreDump();
                    exit(1);
                }
                integerResult = integerOperand1 / integerOperand2;
            }
            else // (operation == "modulo")
            {
                if (integerOperand2 == 0)
                {
                    cout << "HAL9000: modulo division by zero" << endl;
                    CoreDump();
                    exit(1);
                }
                integerResult = integerOperand1 % integerOperand2;
            }
            topAddress = GetKernelVariableIntegerValue("TOP_FUNCTION_CALL_VALUES_STACK_ADDRESS");
            ram.SetP(topAddress);
            ram.Push("integer", itos(integerResult), partitionNo);
            topAddress = ram.GetP();
            SetKernelVariableValue("TOP_FUNCTION_CALL_VALUES_STACK_ADDRESS", itos(topAddress));
        }
        else if (contents2.symbol == "float")
        {
            if (operation == "modulo")
            {
                cout << "HAL9000: second operand in modulo division not an integer" << endl;
                CoreDump();
                exit(1);
            }
            floatOperand2 = atof(contents2.value.c_str());
            if (operation == "add")
            {
                floatResult = integerOperand1 + floatOperand2;
            }
            else if (operation == "subtract")
            {
                floatResult = integerOperand1 - floatOperand2;
            }
            else if (operation == "multiply")
            {
                floatResult = integerOperand1 * floatOperand2;
            }
            else // (operation == "divide")
            {
                if (floatOperand2 == 0.0)
                {
                    cout << "HAL9000: division by zero" << endl;
                    CoreDump();
                    exit(1);
                }
                floatResult = integerOperand1 / floatOperand2;
            }
            topAddress = GetKernelVariableIntegerValue("TOP_FUNCTION_CALL_VALUES_STACK_ADDRESS");
            ram.SetP(topAddress);
            ram.Push("integer", itos(floatResult), partitionNo);
            topAddress = ram.GetP();
            SetKernelVariableValue("TOP_FUNCTION_CALL_VALUES_STACK_ADDRESS", itos(topAddress));
        }
        else
        {
            cout << "HAL9000: second operand in aarithmetic operation is non-numeric type" << endl;
            CoreDump();
            exit(1);
        }
    }
    else if (contents1.symbol == "float")
    {
        if (operation == "modulo")
        {
            cout << "HAL9000: first operand in modulo division not an integer" << endl;
            CoreDump();
            exit(1);
        }
        floatOperand1 = atof(contents1.value.c_str());
        if (contents2.symbol == "float")
        {
            if (operation == "modulo")
            {
                cout << "HAL9000: second operand in modulo division not an integer" << endl;
                CoreDump();
                exit(1);
            }
            floatOperand2 = atof(contents2.value.c_str());
            if (operation == "add")
            {
                floatResult = floatOperand1 + floatOperand2;
            }
            else if (operation == "subtract")
            {
                floatResult = floatOperand1 - floatOperand2;
            }
            else if (operation == "multiply")
            {
                floatResult = floatOperand1 * floatOperand2;
            }
            else // (operation == "divide")
            {
                if (floatOperand2 == 0.0)
                {
                    cout << "HAL9000: division by zero" << endl;
                    CoreDump();
                    exit(1);
                }
                floatResult = floatOperand1 / floatOperand2;
            }
            topAddress = GetKernelVariableIntegerValue("TOP_FUNCTION_CALL_VALUES_STACK_ADDRESS");
            ram.SetP(topAddress);
            ram.Push("float", dtos(floatResult), partitionNo);
            topAddress = ram.GetP();
            SetKernelVariableValue("TOP_FUNCTION_CALL_VALUES_STACK_ADDRESS", itos(topAddress));
        }
        else if (contents2.symbol == "integer")
        {
            integerOperand2 = atoi(contents2.value.c_str());
            if (operation == "add")
            {
                floatResult = floatOperand1 + integerOperand2;
            }
            else if (operation == "subtract")
            {
                floatResult = floatOperand1 - integerOperand2;
            }
            else if (operation == "multiply")
            {
                floatResult = floatOperand1 * integerOperand2;
            }
            else // (operation == "divide")
            {
                if (integerOperand2 == 0)
                {
                    cout << "HAL9000: division by zero" << endl;
                    CoreDump();
                    exit(1);
                }
                floatResult = floatOperand1 / integerOperand2;
            }
            topAddress = GetKernelVariableIntegerValue("TOP_FUNCTION_CALL_VALUES_STACK_ADDRESS");
            ram.SetP(topAddress);
            ram.Push("float", dtos(floatResult), partitionNo);
            topAddress = ram.GetP();
            SetKernelVariableValue("TOP_FUNCTION_CALL_VALUES_STACK_ADDRESS", itos(topAddress));
        }
        else
        {
            cout << "HAL9000: second operand in arithmetic operation is non-numeric type" << endl;
            CoreDump();
            exit(1);
        }
    }
    else
    {
        cout << "HAL9000: first operand in arithmetic operation is non-numeric type" << endl;
        CoreDump();
        exit(1);
    }

    return;
}

void Join()
{
    int functionCallValuesStackStartAddress;
    int segmentSize;
    int topAddress;
    memoryCell contents1;
    memoryCell contents2;
    size_t foundAddress;
    string symbol;

    functionCallValuesStackStartAddress = GetMemorySegmentBoundary("FUNCTION_CALL_VALUES_STACK_START_ADDRESS", segmentSize);
    topAddress = GetKernelVariableIntegerValue("TOP_FUNCTION_CALL_VALUES_STACK_ADDRESS");
    ram.SetP(topAddress);
    if (ram.GetP() > functionCallValuesStackStartAddress)
    {
        cout << "HAL9000: function call values stack segmentation violation" << endl;
        CoreDump();
        exit(1);
    }
    contents2 = ram.Pop(partitionNo);
    topAddress = ram.GetP();
    SetKernelVariableValue("TOP_FUNCTION_CALL_VALUES_STACK_ADDRESS", itos(topAddress));
    foundAddress = contents2.symbol.find("address");
    if (foundAddress != string::npos)
    {
        ram.SetP(atoi(contents2.value.c_str()));
        contents2 = ram.Read(partitionNo);
    }

    topAddress = GetKernelVariableIntegerValue("TOP_FUNCTION_CALL_VALUES_STACK_ADDRESS");
    ram.SetP(topAddress);
    if (ram.GetP() > functionCallValuesStackStartAddress)
    {
        cout << "HAL9000: function call values stack segmentation violation" << endl;
        CoreDump();
        exit(1);
    }
    contents1 = ram.Pop(partitionNo);
    topAddress = ram.GetP();
    SetKernelVariableValue("TOP_FUNCTION_CALL_VALUES_STACK_ADDRESS", itos(topAddress));
    foundAddress = contents1.symbol.find("address");
    if (foundAddress != string::npos)
    {
        ram.SetP(atoi(contents1.value.c_str()));
        contents1 = ram.Read(partitionNo);
    }

    topAddress = GetKernelVariableIntegerValue("TOP_FUNCTION_CALL_VALUES_STACK_ADDRESS");
    ram.SetP(topAddress);
    if (GetDataType(contents2.symbol) == "integer" && GetDataType(contents1.symbol) == "integer")
    {
        symbol = "integer";
    }
    else if (GetDataType(contents2.symbol) == "integer" && GetDataType(contents1.symbol) == "float")
    {
        symbol = "float";
    }
    else
    {
        symbol = "string";
    }
    ram.Push(symbol, contents2.value + contents1.value, partitionNo);
    topAddress = ram.GetP();
    SetKernelVariableValue("TOP_FUNCTION_CALL_VALUES_STACK_ADDRESS", itos(topAddress));

    return;
}

void GetPid(string pidType)
{
    int pid;

    pid = GetKernelVariableIntegerValue(pidType);

    ram.SetP(GetKernelVariableIntegerValue("TOP_FUNCTION_CALL_VALUES_STACK_ADDRESS"));
    ram.Push("integer", itos(pid), partitionNo);
    SetKernelVariableValue("TOP_FUNCTION_CALL_VALUES_STACK_ADDRESS", itos(ram.GetP()));

    return;
}

void CoreSnapShot()
{
    int pid;
    int coreSnapShotCounter = 0;
    struct stat fileStatusBuffer;

    pid = GetKernelVariableIntegerValue("PID");
    while (1)
    {
        coreSnapShotCounter++;
        if (stat((itos(pid) + "_coresnapshot_" + itos(coreSnapShotCounter)).c_str(), &fileStatusBuffer) == -1)
        {
            ProcessImageToFile(pid, "coresnapshot_" + itos(coreSnapShotCounter));
            break;
        }
    }

    return;
}

// System Calls

void Open(string symbol, string restartInstructionStatusFlag, string result)
{
    int pid;
    int address1;
    int address2;
    memoryCell contents;
    string mode;
    bool isLocalSymbol = true;

    address1 = GetLocalSymbolAddress(symbol);
    if (address1 == -1)
    {
        isLocalSymbol = false;
        address1 = GetGlobalSymbolAddress(symbol);
        if (address1 == -1)
        {
            cout << "HAL9000: " << symbol << " not defined" << endl;
            CoreDump();
            exit(1);
        }
    }
    pid = GetKernelVariableIntegerValue("PID");

    ram.SetP(address1);
    contents = ram.Read(partitionNo);
    address2 = atoi(contents.value.c_str());
    ram.SetP(address2);
    contents = ram.Read(partitionNo);
    if (restartInstructionStatusFlag == "false")
    {
        if (contents.symbol == "input_file")
        {
            mode = "input";
        }
        else if (contents.symbol == "output_file")
        {
            mode = "output";
        }
        else
        {
            cout << "HAL9000: cannot open " << symbol << " as it is not a file" << endl;
            CoreDump();
            exit(1);
        }
        if (symbol == "keyboard" || symbol == "display")
        {
            HALosMessage.parameter1 = "OPEN";
            HALosMessage.parameter3 = symbol;
            HALosMessage.parameter4 = "";
            HALosMessage.parameter5 = mode;
            HALosMessage.parameter6 = "";
        }
        else
        {
            address1 = ram.GetP();
            if (isLocalSymbol)
            {
                address1--;
            }
            else
            {
                address1++;
            }
            ram.SetP(address1);
            contents = ram.Read(partitionNo);
            HALosMessage.parameter1 = "OPEN";
            HALosMessage.parameter3 = contents.value;
            HALosMessage.parameter4 = itos(address2);
            HALosMessage.parameter5 = mode;
            HALosMessage.parameter6 = "";
        }
    }
    else
    {
        if (result == "FILE_OPEN_OK")
        {
            contents.value = "open";
            ram.ReWrite("", contents.value, partitionNo);
        }
        else if (result == "FILE_OPEN_FAILED")
        {
            cout << "HAL9000: unable to open " << symbol << endl;
            CoreDump();
            exit(1);
        }
        else // (result == "FILE_ALREADY_OPEN")
        {
            cout << "HAL9000: " << symbol << " already open" << endl;
            CoreDump();
            exit(1);
        }
    }

    return;
}

void Read(string symbol, string restartInstructionStatusFlag, string buffer, string result)
{
    int pid;
    int address1;
    int address2;
    memoryCell contents;
    bool isLocalSymbol = true;

    address1 = GetLocalSymbolAddress(symbol);
    if (address1 == -1)
    {
        isLocalSymbol = false;
        address1 = GetGlobalSymbolAddress(symbol);
        if (address1 == -1)
        {
            cout << "HAL9000: " << symbol << " not defined" << endl;
            CoreDump();
            exit(1);
        }
    }
    pid = GetKernelVariableIntegerValue("PID");

    ram.SetP(address1);
    contents = ram.Read(partitionNo);
    address2 = atoi(contents.value.c_str());
    ram.SetP(address2);
    contents = ram.Read(partitionNo);
    if (restartInstructionStatusFlag == "false")
    {
        if (contents.symbol != "input_file")
        {
            cout << "HAL9000: cannot read from " << symbol << " as it is not an input file" << endl;
            CoreDump();
            exit(1);
        }
        if (contents.value != "open")
        {
            cout << "HAL9000: cannot read from " << symbol << " as it is not open" << endl;
            CoreDump();
            exit(1);
        }
        if (symbol == "keyboard")
        {
            HALosMessage.parameter1 = "READ";
            HALosMessage.parameter3 = symbol;
            HALosMessage.parameter4 = "";
            HALosMessage.parameter5 = "";
            HALosMessage.parameter6 = "";
        }
        else
        {
            address1 = ram.GetP();
            if (isLocalSymbol)
            {
                address1--;
            }
            else
            {
                address1++;
            }
            ram.SetP(address1);
            contents = ram.Read(partitionNo);
            HALosMessage.parameter1 = "READ";
            HALosMessage.parameter3 = contents.value;
            HALosMessage.parameter4 = itos(address2);
            HALosMessage.parameter5 = "";
            HALosMessage.parameter6 = "";
        }
    }
    else
    {
        if (result == "KEYBOARD_READ_OK" || result == "FILE_READ_OK")
        {
            ram.SetP(GetKernelVariableIntegerValue("TOP_FUNCTION_CALL_VALUES_STACK_ADDRESS"));
            ram.Push("string", buffer, partitionNo);
            SetKernelVariableValue("TOP_FUNCTION_CALL_VALUES_STACK_ADDRESS", itos(ram.GetP()));
        }
        else if (result == "FILE_AT_END")
        {
            ram.SetP(GetKernelVariableIntegerValue("TOP_FUNCTION_CALL_VALUES_STACK_ADDRESS"));
            ram.Push("eof", "", partitionNo);
            SetKernelVariableValue("TOP_FUNCTION_CALL_VALUES_STACK_ADDRESS", itos(ram.GetP()));
        }
        else // (result == "FILE_READ_FAILED")
        {
            cout << "HAL9000: unable to read from " << symbol << endl;
            CoreDump();
            exit(1);
        }
    }

    return;
}

void Write(string symbol, string restartInstructionStatusFlag, string result)
{
    int pid;
    int address1;
    int address2;
    memoryCell contents1;
    memoryCell contents2;
    bool isLocalSymbol = true;

    address1 = GetLocalSymbolAddress(symbol);
    if (address1 == -1)
    {
        isLocalSymbol = false;
        address1 = GetGlobalSymbolAddress(symbol);
        if (address1 == -1)
        {
            cout << "HAL9000: " << symbol << " not defined" << endl;
            CoreDump();
            exit(1);
        }
    }
    pid = GetKernelVariableIntegerValue("PID");

    ram.SetP(address1);
    contents1 = ram.Read(partitionNo);
    address2 = atoi(contents1.value.c_str());
    ram.SetP(address2);
    contents1 = ram.Read(partitionNo);
    address1 = ram.GetP();
    if (isLocalSymbol)
    {
        address1--;
    }
    else
    {
        address1++;
    }
    if (restartInstructionStatusFlag == "false")
    {
        if (contents1.symbol != "output_file")
        {
            cout << "HAL9000: cannot write to " << symbol << " as it is not an output file" << endl;
            CoreDump();
            exit(1);
        }
        if (contents1.value != "open")
        {
            cout << "HAL9000: cannot write to " << symbol << " as it is not open" << endl;
            CoreDump();
            exit(1);
        }
        ram.SetP(GetKernelVariableIntegerValue("TOP_FUNCTION_CALL_VALUES_STACK_ADDRESS"));
        contents1 = ram.Pop(partitionNo);
        SetKernelVariableValue("TOP_FUNCTION_CALL_VALUES_STACK_ADDRESS", itos(ram.GetP()));
        if (contents1.symbol == "float")
        {
            if (IsInteger(contents1.value))
            {
                contents1.value = contents1.value + ".0";
            }
        }
        if (symbol == "display")
        {
            HALosMessage.parameter1 = "WRITE";
            HALosMessage.parameter3 = symbol;
            HALosMessage.parameter4 = "";
            HALosMessage.parameter5 = contents1.value;
            HALosMessage.parameter6 = "";
        }
        else
        {
            ram.SetP(address1);
            contents2 = ram.Read(partitionNo);
            HALosMessage.parameter1 = "WRITE";
            HALosMessage.parameter3 = contents2.value;
            HALosMessage.parameter4 = itos(address2);
            HALosMessage.parameter5 = contents1.value;
            HALosMessage.parameter6 = "";
        }
    }
    else
    {
        if (result == "FILE_WRITE_FAILED")
        {
            cout << "HAL9000: unable to write to " << symbol << endl;
            CoreDump();
            exit(1);
        }
    }

    return;
}

void Newline(string symbol, string restartInstructionStatusFlag, string result)
{
    int pid;
    int address1;
    int address2;
    memoryCell contents;
    bool isLocalSymbol = true;

    address1 = GetLocalSymbolAddress(symbol);
    if (address1 == -1)
    {
        isLocalSymbol = false;
        address1 = GetGlobalSymbolAddress(symbol);
        if (address1 == -1)
        {
            cout << "HAL9000: " << symbol << " not defined" << endl;
            CoreDump();
            exit(1);
        }
    }
    pid = GetKernelVariableIntegerValue("PID");

    ram.SetP(address1);
    contents = ram.Read(partitionNo);
    address2 = atoi(contents.value.c_str());
    ram.SetP(address2);
    contents = ram.Read(partitionNo);
    if (restartInstructionStatusFlag == "false")
    {
        if (contents.symbol != "output_file")
        {
            cout << "HAL9000: cannot newline to " << symbol << " as it is not an output file" << endl;
            CoreDump();
            exit(1);
        }
        if (contents.value != "open")
        {
            cout << "HAL9000: cannot newline to " << symbol << " as it is not open" << endl;
            CoreDump();
            exit(1);
        }
        if (symbol == "display")
        {
            HALosMessage.parameter1 = "NEWLINE";
            HALosMessage.parameter3 = symbol;
            HALosMessage.parameter4 = "";
            HALosMessage.parameter5 = "";
            HALosMessage.parameter6 = "";
        }
        else
        {
            address1 = ram.GetP();
            if (isLocalSymbol)
            {
                address1--;
            }
            else
            {
                address1++;
            }
            ram.SetP(address1);
            contents = ram.Read(partitionNo);
            HALosMessage.parameter1 = "NEWLINE";
            HALosMessage.parameter3 = contents.value;
            HALosMessage.parameter4 = itos(address2);
            HALosMessage.parameter5 = "";
            HALosMessage.parameter6 = "";
        }
    }
    else
    {
        if (result == "FILE_NEWLINE_FAILED")
        {
            cout << "HAL9000: unable to newline to " << symbol << endl;
            CoreDump();
            exit(1);
        }
    }

    return;
}

void Close(string symbol, string restartInstructionStatusFlag, string result)
{
    int pid;
    int address1;
    int address2;
    memoryCell contents;
    bool isLocalSymbol = true;

    address1 = GetLocalSymbolAddress(symbol);
    if (address1 == -1)
    {
        isLocalSymbol = false;
        address1 = GetGlobalSymbolAddress(symbol);
        if (address1 == -1)
        {
            cout << "HAL9000: " << symbol << " not defined" << endl;
            CoreDump();
            exit(1);
        }
    }
    pid = GetKernelVariableIntegerValue("PID");

    ram.SetP(address1);
    contents = ram.Read(partitionNo);
    address2 = atoi(contents.value.c_str());
    ram.SetP(address2);
    contents = ram.Read(partitionNo);
    if (restartInstructionStatusFlag == "false")
    {
        if (contents.symbol != "input_file" && contents.symbol != "output_file")
        {
            cout << "HAL9000: cannot close " << symbol << " as it is not a file" << endl;
            CoreDump();
            exit(1);
        }
        if (symbol == "keyboard" || symbol == "display")
        {
            HALosMessage.parameter1 = "CLOSE";
            HALosMessage.parameter3 = symbol;
            HALosMessage.parameter4 = "";
            HALosMessage.parameter5 = "";
            HALosMessage.parameter6 = "";
            ;
        }
        else
        {
            address1 = ram.GetP();
            if (isLocalSymbol)
            {
                address1--;
            }
            else
            {
                address1++;
            }
            ram.SetP(address1);
            contents = ram.Read(partitionNo);
            HALosMessage.parameter1 = "CLOSE";
            HALosMessage.parameter3 = contents.value;
            HALosMessage.parameter4 = itos(address2);
            HALosMessage.parameter5 = "";
            HALosMessage.parameter6 = "";
        }
    }
    else
    {
        if (result == "FILE_CLOSE_OK")
        {
            contents.value = "closed";
            ram.ReWrite("", contents.value, partitionNo);
        }
        else // close failed for some reason
        {
            cout << "HAL9000: unable to close " << symbol << endl;
            CoreDump();
            exit(1);
        }
    }

    return;
}

void Clone(string restartInstructionStatusFlag, string buffer, string result)
{
    if (restartInstructionStatusFlag == "false")
    {
        SetKernelVariableValue("RESTART_INSTRUCTION_STATUS_FLAG", "true");
        ProcessImageToFile(GetKernelVariableIntegerValue("PID"), "backingstore");
        SetKernelVariableValue("RESTART_INSTRUCTION_STATUS_FLAG", "false");
        HALosMessage.parameter1 = "CLONE";
        HALosMessage.parameter3 = "";
        HALosMessage.parameter4 = "";
        HALosMessage.parameter5 = "";
        HALosMessage.parameter6 = "";
    }
    else
    {
        if (result == "CLONE_OK")
        {
            ram.SetP(GetKernelVariableIntegerValue("TOP_FUNCTION_CALL_VALUES_STACK_ADDRESS"));
            ram.Push("integer", buffer, partitionNo);
            SetKernelVariableValue("TOP_FUNCTION_CALL_VALUES_STACK_ADDRESS", itos(ram.GetP()));
        }
        else // (result == "CLONE_FAILED")
        {
            cout << "HAL9000: unable to clone child process" << endl;
            CoreDump();
            exit(1);
        }
    }

    return;
}

void Run(string symbol, string restartInstructionStatusFlag)
{
    int address1;
    int address2;
    memoryCell contents;
    string commandLine;

    if (restartInstructionStatusFlag == "false")
    {
        if (symbol[0] == '\'')
        {
            commandLine = symbol.substr(1, symbol.length() - 1);
        }
        else
        {
            address1 = GetLocalSymbolAddress(symbol);
            if (address1 == -1)
            {
                address1 = GetGlobalSymbolAddress(symbol);
                if (address1 == -1)
                {
                    cout << "HAL9000: " << symbol << " not defined" << endl;
                    CoreDump();
                    exit(1);
                }
            }
            ram.SetP(address1);
            contents = ram.Read(partitionNo);
            address2 = atoi(contents.value.c_str());
            ram.SetP(address2);
            contents = ram.Read(partitionNo);
            commandLine = contents.value;
        }
        HALosMessage.parameter1 = "RUN";
        HALosMessage.parameter3 = commandLine;
        HALosMessage.parameter4 = itos(GetKernelVariableIntegerValue("PARENT_PID"));
        HALosMessage.parameter5 = "";
        HALosMessage.parameter6 = "";
    }
    else // (result == "RUN_FAILED")
    {
        cout << "HAL9000: unable to replace process image" << endl;
        CoreDump();
        exit(1);
    }

    return;
}

// The "Backing Store"

void ProcessImageToMemory(int pid, int quantumLength)
{
    ifstream processImageFile;
    string processImageFileName;
    int globalSymbolsTableStartAddress;
    int globalSymbolsTableEndAddress;
    int functionCallStackEndAddress;
    int functionCallStackStartAddress;
    int segmentSize;
    char uselessCharacter;
    int address;
    char fieldSeparator;
    string symbol;
    string value;
    size_t foundLowerCaseAddress;
    size_t foundUpperCaseAddress;
    size_t foundInstructionPointer;
    int clockTicks;

    clockTicks = GetClockTicks();
    clockTicks = clockTicks + 10;
    SetClockTicks(clockTicks);

    processImageFileName = itos(pid) + "_backingstore";
    processImageFile.open(processImageFileName.c_str());
    if (!processImageFile)
    {
        cout << "HAL9000: unable to swap in process " << pid << " image" << endl;
        CoreDump();
        exit(1);
    }

    globalSymbolsTableStartAddress = GetMemorySegmentBoundary("GLOBAL_SYMBOLS_TABLE_START_ADDRESS", segmentSize);
    globalSymbolsTableEndAddress = globalSymbolsTableStartAddress + segmentSize;
    functionCallStackEndAddress = GetMemorySegmentBoundary("FUNCTION_CALL_STACK_START_ADDRESS", segmentSize) + 1;
    functionCallStackStartAddress = functionCallStackEndAddress - segmentSize;

    ram.Clear(0, partitionNo);

    processImageFile >> uselessCharacter;
    while (processImageFile)
    {
        processImageFile >> uselessCharacter;
        processImageFile >> uselessCharacter;
        processImageFile >> address;
        ram.SetP(address);
        processImageFile >> fieldSeparator;
        processImageFile >> symbol;
        processImageFile.ignore(256, ':');
        foundLowerCaseAddress = symbol.find("address");
        foundUpperCaseAddress = symbol.find("ADDRESS");
        foundInstructionPointer = symbol.find("INSTRUCTION_POINTER");
        if (foundLowerCaseAddress != string::npos ||
            foundUpperCaseAddress != string::npos ||
            foundInstructionPointer != string::npos)
        {
            processImageFile >> uselessCharacter;
            processImageFile >> uselessCharacter;
            processImageFile >> uselessCharacter;
        }
        else
        {
            if (symbol.length() > 0)
            {
                if (address >= globalSymbolsTableStartAddress && address < globalSymbolsTableEndAddress)
                {
                    processImageFile >> uselessCharacter;
                    processImageFile >> uselessCharacter;
                    processImageFile >> uselessCharacter;
                }
                else if (address >= functionCallStackStartAddress && address < functionCallStackEndAddress)
                {
                    processImageFile >> uselessCharacter;
                    processImageFile >> uselessCharacter;
                    processImageFile >> uselessCharacter;
                }
            }
        }
        getline(processImageFile, value);
        ram.Write(symbol, value, partitionNo);
        processImageFile >> uselessCharacter;
    }

    processImageFile.close();

    SetKernelVariableValue("RUNNING_TIME", itos(0));
    SetKernelVariableValue("QUANTUM_LENGTH", itos(quantumLength));
    SetKernelVariableValue("QUANTUM_TIME_REMAINING", itos(quantumLength));

    return;
}

void ProcessImageToFile(int pid, string location)
{
    ofstream processImageFile;
    string processImageFileName;
    memoryCell contents;
    int i;
    int globalSymbolsTableStartAddress;
    int globalSymbolsTableEndAddress;
    int functionCallStackEndAddress;
    int functionCallStackStartAddress;
    int segmentSize;
    int clockTicks;

    if (location == "backingstore")
    {
        clockTicks = GetClockTicks();
        clockTicks = clockTicks + 15;
        SetClockTicks(clockTicks);
    }

    processImageFileName = itos(pid) + "_" + location;
    processImageFile.open(processImageFileName.c_str());
    if (!processImageFile)
    {
        cout << "HAL9000: unable to swap out process " << pid << " image" << endl;
        CoreDump();
        exit(1);
    }

    globalSymbolsTableStartAddress = GetMemorySegmentBoundary("GLOBAL_SYMBOLS_TABLE_START_ADDRESS", segmentSize);
    globalSymbolsTableEndAddress = globalSymbolsTableStartAddress + segmentSize;
    functionCallStackEndAddress = GetMemorySegmentBoundary("FUNCTION_CALL_STACK_START_ADDRESS", segmentSize) + 1;
    functionCallStackStartAddress = functionCallStackEndAddress - segmentSize;

    for (i = 0; i < PARTITION_SIZE; i++)
    {
        ram.SetP(i);
        contents = ram.Read(partitionNo);
        if (contents.symbol.length() > 0)
        {
            processImageFile << "0d_";
            processImageFile << i << ": ";
            processImageFile << contents.symbol << " :";
            if (AddressField(contents.symbol, i,
                             globalSymbolsTableStartAddress,
                             globalSymbolsTableEndAddress,
                             functionCallStackStartAddress,
                             functionCallStackEndAddress))
            {
                processImageFile << "0d_";
            }
            processImageFile << contents.value << endl;
        }
    }

    processImageFile.close();

    return;
}

bool AddressField(string symbol, int i, int globalSymbolsTableStartAddress, int globalSymbolsTableEndAddress,
                  int functionCallStackStartAddress, int functionCallStackEndAddress)
{
    size_t foundLowerCaseAddress;
    size_t foundUpperCaseAddress;
    size_t foundInstructionPointer;

    foundLowerCaseAddress = symbol.find("address");
    foundUpperCaseAddress = symbol.find("ADDRESS");
    foundInstructionPointer = symbol.find("INSTRUCTION_POINTER");
    if (foundLowerCaseAddress != string::npos ||
        foundUpperCaseAddress != string::npos ||
        foundInstructionPointer != string::npos)
    {
        return true;
    }
    else
    {
        if (symbol.length() > 0)
        {
            if (i >= globalSymbolsTableStartAddress && i < globalSymbolsTableEndAddress)
            {
                return true;
            }
            else if (i >= functionCallStackStartAddress && i < functionCallStackEndAddress)
            {
                return true;
            }
        }
    }

    return false;
}

// Local and Global Variable Interfaces

void AllocateLocalSymbol(memoryCell contents1)
{
    int functionCallValuesStackStartAddress;
    int topFunctionCallValuesStackAddress;
    int functionCallStackStartAddress;
    int topFunctionCallStackAddress;
    int segmentSize;
    memoryCell contents2;

    functionCallValuesStackStartAddress = GetMemorySegmentBoundary("FUNCTION_CALL_VALUES_STACK_START_ADDRESS", segmentSize);
    topFunctionCallValuesStackAddress = GetKernelVariableIntegerValue("TOP_FUNCTION_CALL_VALUES_STACK_ADDRESS");
    ram.SetP(topFunctionCallValuesStackAddress);
    if (ram.GetP() == functionCallValuesStackStartAddress - segmentSize)
    {
        cout << "HAL9000: function call values stack segmentation violation" << endl;
        CoreDump();
        exit(1);
    }
    if (contents1.symbol == "constant")
    {
        ram.Push("constant", "undefined_type", partitionNo);
    }
    else if (contents1.symbol == "variable")
    {
        ram.Push("variable", "undefined_type", partitionNo);
    }
    else if (contents1.symbol == "file")
    {
        if (contents1.value == "keyboard")
        {
            ram.Push("input_" + contents1.symbol, "closed", partitionNo);
        }
        else if (contents1.value == "display")
        {
            ram.Push("output_" + contents1.symbol, "closed", partitionNo);
        }
        else
        {
            ram.Push(contents1.symbol, "closed", partitionNo);
        }
    }
    else // (contents1.symbol == "reference")
    {
        contents2 = ram.Pop(partitionNo);
    }
    topFunctionCallValuesStackAddress = ram.GetP();
    SetKernelVariableValue("TOP_FUNCTION_CALL_VALUES_STACK_ADDRESS", itos(topFunctionCallValuesStackAddress));

    functionCallStackStartAddress = GetMemorySegmentBoundary("FUNCTION_CALL_STACK_START_ADDRESS", segmentSize);
    topFunctionCallStackAddress = GetKernelVariableIntegerValue("TOP_FUNCTION_CALL_STACK_ADDRESS");
    ram.SetP(topFunctionCallStackAddress);
    if (ram.GetP() == functionCallStackStartAddress - segmentSize)
    {
        cout << "HAL9000: function call stack segmentation violation" << endl;
        return;
    }
    if (contents1.symbol == "constant" ||
        contents1.symbol == "variable" ||
        contents1.symbol == "file")
    {
        ram.Push(contents1.value, itos(topFunctionCallValuesStackAddress), partitionNo);
    }
    else // (contents1.symbol == "reference")
    {
        ram.Push(contents1.value, contents2.value, partitionNo);
    }
    topFunctionCallStackAddress = ram.GetP();
    SetKernelVariableValue("TOP_FUNCTION_CALL_STACK_ADDRESS", itos(topFunctionCallStackAddress));

    if (contents1.symbol == "file")
    {
        if (contents1.value != "keyboard" && contents1.value != "display")
        {
            functionCallValuesStackStartAddress = GetMemorySegmentBoundary("FUNCTION_CALL_VALUES_STACK_START_ADDRESS", segmentSize);
            topFunctionCallValuesStackAddress = GetKernelVariableIntegerValue("TOP_FUNCTION_CALL_VALUES_STACK_ADDRESS");
            ram.SetP(topFunctionCallValuesStackAddress);
            if (ram.GetP() == functionCallValuesStackStartAddress - segmentSize)
            {
                cout << "HAL9000: function call values stack segmentation violation" << endl;
                CoreDump();
                exit(1);
            }
            ram.Push("file_name", "", partitionNo);
            topFunctionCallValuesStackAddress = ram.GetP();
            SetKernelVariableValue("TOP_FUNCTION_CALL_VALUES_STACK_ADDRESS", itos(topFunctionCallValuesStackAddress));
        }
    }

    return;
}

void AssignTypeToLocalSymbol(memoryCell contents)
{
    string symbol;
    string type;

    symbol = contents.symbol;
    type = contents.value;

    ram.SetP(GetKernelVariableIntegerValue("TOP_FUNCTION_CALL_STACK_ADDRESS"));
    contents = ram.Read(partitionNo);
    while (1)
    {
        if (contents.symbol.substr(0, 5) == "call_")
        {
            cout << "HAL9000: cannot set data type for undeclared local symbol " << symbol << endl;
            CoreDump();
            exit(1);
        }
        if (contents.symbol == symbol)
        {
            ram.SetP(atoi(contents.value.c_str()));
            contents = ram.Read(partitionNo);
            if (contents.symbol == "constant")
            {
                if (type == "integer")
                {
                    ram.ReWrite(contents.symbol + "_" + type, "?#@NULL_VALUE@#?", partitionNo);
                }
                else if (type == "float")
                {
                    ram.ReWrite(contents.symbol + "_" + type, "?#@NULL_VALUE@#?", partitionNo);
                }
                else if (type == "string")
                {
                    ram.ReWrite(contents.symbol + "_" + type, "?#@NULL_VALUE@#?", partitionNo);
                }
                else
                {
                    cout << "HAL9000: unrecognized data type " << type << " for local symbol " << symbol << endl;
                    CoreDump();
                    exit(1);
                }
            }
            else if (contents.symbol == "variable")
            {
                if (type == "integer")
                {
                    ram.ReWrite(contents.symbol + "_" + type, "0", partitionNo);
                }
                else if (type == "float")
                {
                    ram.ReWrite(contents.symbol + "_" + type, "0.0", partitionNo);
                }
                else if (type == "string")
                {
                    ram.ReWrite(contents.symbol + "_" + type, "", partitionNo);
                }
                else
                {
                    cout << "HAL9000: unrecognized data type " << type << " for local symbol " << symbol << endl;
                    CoreDump();
                    exit(1);
                }
            }
            else if (contents.symbol == "file")
            {
                if (type == "input")
                {
                    ram.ReWrite(type + "_" + contents.symbol, "closed", partitionNo);
                }
                else if (type == "output")
                {
                    ram.ReWrite(type + "_" + contents.symbol, "closed", partitionNo);
                }
                else
                {
                    cout << "HAL9000: unrecognized data type " << type << " for local symbol " << symbol << endl;
                    exit(1);
                }
                ram.IterateDown();
                contents = ram.Read(partitionNo);
                ram.ReWrite(type + "_" + contents.symbol, "", partitionNo);
            }
            break;
        }
        ram.IterateUp();
        contents = ram.Read(partitionNo);
    }

    return;
}

void AllocateLocalArray(memoryCell contents)
{
    int functionCallValuesStackStartAddress;
    int topFunctionCallValuesStackAddress;
    int segmentSize;
    int noOfArrayElements = atoi(contents.value.c_str());
    string symbol;
    int i;

    functionCallValuesStackStartAddress = GetMemorySegmentBoundary("FUNCTION_CALL_VALUES_STACK_START_ADDRESS", segmentSize);
    topFunctionCallValuesStackAddress = GetKernelVariableIntegerValue("TOP_FUNCTION_CALL_VALUES_STACK_ADDRESS");
    ram.SetP(topFunctionCallValuesStackAddress);
    if (ram.GetP() == functionCallValuesStackStartAddress - noOfArrayElements - segmentSize)
    {
        cout << "HAL9000: function call values stack segmentation violation" << endl;
        CoreDump();
        exit(1);
    }
    contents = ram.Read(partitionNo);
    symbol = contents.symbol;
    contents.symbol = contents.symbol + "_array_0";
    ram.ReWrite(contents.symbol, contents.value, partitionNo);

    for (i = 1; i < noOfArrayElements; i++)
    {
        ram.SetP(GetKernelVariableIntegerValue("TOP_FUNCTION_CALL_VALUES_STACK_ADDRESS"));
        contents.symbol = symbol + "_array_" + itos(i);
        ram.Push(contents.symbol, contents.value, partitionNo);
        SetKernelVariableValue("TOP_FUNCTION_CALL_VALUES_STACK_ADDRESS", itos(ram.GetP()));
    }

    return;
}

int GetLocalSymbolAddress(string symbol)
{
    memoryCell contents;

    ram.SetP(GetKernelVariableIntegerValue("TOP_FUNCTION_CALL_STACK_ADDRESS"));
    contents = ram.Read(partitionNo);
    while (1)
    {
        if (contents.symbol == symbol)
        {
            return ram.GetP();
        }
        else if (contents.symbol.substr(0, 5) == "call_")
        {
            return -1;
        }
        ram.IterateUp();
        contents = ram.Read(partitionNo);
    }
}

int GetGlobalSymbolAddress(string symbol)
{
    int globalSymbolsTableStartAddress;
    int lastGlobalSymbolsTableAddress;
    int segmentSize;
    memoryCell contents;

    globalSymbolsTableStartAddress = GetMemorySegmentBoundary("GLOBAL_SYMBOLS_TABLE_START_ADDRESS", segmentSize);
    lastGlobalSymbolsTableAddress = GetKernelVariableIntegerValue("LAST_GLOBAL_SYMBOLS_TABLE_ADDRESS");
    ram.SetP(globalSymbolsTableStartAddress);
    contents = ram.Read(partitionNo);
    while (ram.GetP() <= lastGlobalSymbolsTableAddress)
    {
        if (contents.symbol == symbol)
        {
            return ram.GetP();
        }
        ram.IterateUp();
        contents = ram.Read(partitionNo);
    }

    return -1;
}

string GetDataType(string dataType)
{
    size_t foundDataType;

    foundDataType = dataType.find("integer");
    if (foundDataType != string::npos)
    {
        return ("integer");
    }

    foundDataType = dataType.find("float");
    if (foundDataType != string::npos)
    {
        return ("float");
    }

    foundDataType = dataType.find("string");
    if (foundDataType != string::npos)
    {
        return ("string");
    }

    foundDataType = dataType.find("input_file");
    if (foundDataType != string::npos)
    {
        return ("input_file");
    }

    foundDataType = dataType.find("output_file");
    if (foundDataType != string::npos)
    {
        return ("output_file");
    }

    cout << "HAL9000: " << dataType << " is unrecognized data type " << endl;
    CoreDump();
    exit(1);
}

string GetDataTypeCategory(string dataTypeCategory)
{
    size_t foundDataTypeCategory;

    foundDataTypeCategory = dataTypeCategory.find("constant");
    if (foundDataTypeCategory != string::npos)
    {
        return ("constant");
    }

    foundDataTypeCategory = dataTypeCategory.find("variable");
    if (foundDataTypeCategory != string::npos)
    {
        return ("variable");
    }

    cout << "HAL9000: " << dataTypeCategory << " is unrecognized or invalid data type category" << endl;
    CoreDump();
    exit(1);
}

bool IsInteger(string value)
{
    int i;

    if (value.length() == 0)
    {
        return false;
    }

    for (i = 0; i < value.length(); i++)
    {
        if (i == 0 && value[i] == '-')
        {
            continue;
        }
        else if (!isdigit(value[i]))
        {
            return false;
        }
    }

    return true;
}

bool IsFloat(string value)
{
    int i;
    bool decimalPointSeen = false;

    if (value.length() == 0)
    {
        return false;
    }

    for (i = 0; i < value.length(); i++)
    {
        if (i == 0 && value[i] == '-')
        {
            continue;
        }
        else if (value[i] == '.')
        {
            if (!decimalPointSeen)
            {
                decimalPointSeen = true;
                continue;
            }
            else
            {
                return false;
            }
        }
        else if (!isdigit(value[i]))
        {
            return false;
        }
    }

    return true;
}

// Kernel Variable Interfaces

int GetKernelVariableIntegerValue(string kernelVariableDescription)
{
    int startAddress;
    int segmentSize;
    memoryCell contents;
    int kernelVariableValue;

    startAddress = GetMemorySegmentBoundary("KERNEL_SPACE_START_ADDRESS", segmentSize);
    startAddress = startAddress + segmentSize;
    ram.SetP(startAddress);
    ram.IterateDown();
    while (1)
    {
        contents = ram.Read(partitionNo);
        if (contents.symbol == kernelVariableDescription)
        {
            break;
        }
        ram.IterateDown();
    }
    kernelVariableValue = atoi(contents.value.c_str());

    return (kernelVariableValue);
}

string GetKernelVariableStringValue(string kernelVariableDescription)
{
    int startAddress;
    int segmentSize;
    memoryCell contents;
    string kernelVariableValue;

    startAddress = GetMemorySegmentBoundary("KERNEL_SPACE_START_ADDRESS", segmentSize);
    startAddress = startAddress + segmentSize;
    ram.SetP(startAddress);
    ram.IterateDown();
    while (1)
    {
        contents = ram.Read(partitionNo);
        if (contents.symbol == kernelVariableDescription)
        {
            break;
        }
        ram.IterateDown();
    }
    kernelVariableValue = contents.value;

    return (kernelVariableValue);
}

void SetKernelVariableValue(string kernelVariableDescription, string value)
{
    int startAddress;
    int segmentSize;
    memoryCell contents;

    startAddress = GetMemorySegmentBoundary("KERNEL_SPACE_START_ADDRESS", segmentSize);
    startAddress = startAddress + segmentSize;
    ram.SetP(startAddress);
    ram.IterateDown();
    while (1)
    {
        contents = ram.Read(partitionNo);
        if (contents.symbol == kernelVariableDescription)
        {
            break;
        }
        ram.IterateDown();
    }
    ram.ReWrite("", value, partitionNo);

    return;
}

// Numeric to String Functions

string itos(int i)
{
    stringstream s;

    s << i;

    return s.str();
}

string dtos(double d)
{
    stringstream s;

    s << d;

    return s.str();
}
