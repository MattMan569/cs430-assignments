//
// HALcompile.cpp
//
// Copyright (c) 2019 Robert J. Hilderman.
// All Rights Reserved.
//

#include "HALcompile.h"

void Compile (string arguments [])
{
    string programName;
    ifstream programSourceCodeFile;
    int programTextStartAddress;
    int currentProgramTextAddress;
    string symbol;
    string value;
    memoryCell contents1;
    memoryCell contents2;
    bool mainSeen = false;
    bool functionSeen = false;
    int address;
    size_t foundArrayBracket;
    static string arraySize = "";
    size_t arraySizeStartPosition;
    size_t arraySizeEndPosition;
    int segmentSize;
    string result;
    size_t fileExtensionPosition;
    int lineNo = 0;

    if (!GetProcessImageTemplate ())
    {
        SendReturnStatusToHALshell (itos (nextPid), "error", "no process image template");
        nextPid ++;
        return;
    }

    if (arguments [0] == "")
    {
        SendReturnStatusToHALshell (itos (nextPid), "error", "no program name specified");
        nextPid ++;
        return;
    }

    if (arguments [1] != "")
    {
        SendReturnStatusToHALshell (itos (nextPid), "error", "compile requires only one argument (the program name)");
        nextPid ++;
        return;
    }

    programName = arguments [0];
    fileExtensionPosition = programName.find (".hal");
    if (fileExtensionPosition == string::npos)
    {
        programName = programName + ".hal";
    }
    else if (programName.substr (fileExtensionPosition, 4) != programName.substr ((programName.length () - 4), 4))
    {
        programName = programName + ".hal";
    }

    if (programName [0] == '/')
    {
        programName = programName.substr (1);
    }

    programSourceCodeFile.open (programName.c_str ());
    if (!programSourceCodeFile)
    {
        SendReturnStatusToHALshell (itos (nextPid), "error", "program not found");
        nextPid ++;
        return;
    }

    SetKernelVariableValue ("PROGRAM_NAME", programName);
    programTextStartAddress = GetMemorySegmentBoundary ("PROGRAM_TEXT_START_ADDRESS", segmentSize);
    SetKernelVariableValue ("START_ADDRESS", itos (programTextStartAddress));
    ram.Clear (programTextStartAddress, 0);
    ram.SetP (programTextStartAddress);

    programSourceCodeFile >> symbol;
    while (programSourceCodeFile)
    {
        if (ram.GetP () == programTextStartAddress + segmentSize)
        {
            programSourceCodeFile.close ();
            SendReturnStatusToHALshell (itos (nextPid), "error", "line " + itos (lineNo) + ": size of program source code exceeds " + itos (segmentSize) + " lines");
            nextPid ++;
            return;
        }
        programSourceCodeFile.ignore (256, ':');
        getline (programSourceCodeFile, value);
        lineNo ++;
        if (symbol != "comment")
        {
            if (symbol == "function" ||
                symbol == "label")
            {
                if (symbol == "function")
                {
                    functionSeen = true;
                }
                if (value == "main")
                {
                    mainSeen = true;
                }
                currentProgramTextAddress = ram.GetP ();
                contents1.symbol = symbol;
                contents1.value = value;
                result = AllocateGlobalSymbol (contents1, currentProgramTextAddress);
                if (result != "ok")
                {
                    programSourceCodeFile.close ();
                    SendReturnStatusToHALshell (itos (nextPid), "error", "line " + itos (lineNo) + ": " + result);
                    nextPid ++;
                    return;
                }
                ram.SetP (currentProgramTextAddress);
            }
            else if (symbol == "file")
            {
                if (!functionSeen)
                {
                    currentProgramTextAddress = ram.GetP ();
                    contents1.symbol = symbol;
                    contents1.value = value;
                    result = AllocateGlobalSymbol (contents1, currentProgramTextAddress);
                    if (result != "ok")
                    {
                        programSourceCodeFile.close ();
                        SendReturnStatusToHALshell (itos (nextPid), "error", "line " + itos (lineNo) + ": " + result);
                        nextPid ++;
                        return;
                    }
                    ram.SetP (currentProgramTextAddress);
                }
            }
            else if (symbol == "variable" ||
                     symbol == "constant")
            {
                if (!functionSeen)
                {
                    currentProgramTextAddress = ram.GetP ();
                    contents1.symbol = symbol;
                    contents1.value = value;
                    foundArrayBracket = contents1.value.find ("<");
                    if (foundArrayBracket != string::npos)
                    {
                        arraySizeStartPosition = foundArrayBracket + 1;
                        arraySizeEndPosition = contents1.value.find (">") - 1;
                        arraySize = contents1.value.substr (arraySizeStartPosition, arraySizeEndPosition - arraySizeStartPosition + 1);
                        contents1.value = contents1.value.substr (0, foundArrayBracket);
                    }
                    result = AllocateGlobalSymbol (contents1, currentProgramTextAddress);
                    if (result != "ok")
                    {
                        programSourceCodeFile.close ();
                        SendReturnStatusToHALshell (itos (nextPid), "error", "line " + itos (lineNo) + ": " + result);
                        nextPid ++;
                        return;
                    }
                    ram.SetP (currentProgramTextAddress);
                }
            }
            else if (symbol == "push")
            {
                if (!functionSeen)
                {
                    currentProgramTextAddress = ram.GetP ();
                    contents1.symbol = symbol;
                    contents1.value = value;
                    result = Push (contents1);
                    if (result != "ok")
                    {
                        programSourceCodeFile.close ();
                        SendReturnStatusToHALshell (itos (nextPid), "error", "line " + itos (lineNo) + ": " + result);
                        nextPid ++;
                        return;
                    }
                    ram.SetP (currentProgramTextAddress);
                }
            }
            else if (symbol == "set")
            {
                if (!functionSeen)
                {
                    currentProgramTextAddress = ram.GetP ();
                    contents1.symbol = symbol;
                    contents1.value = value;
                    result = Set (contents1);
                    if (result != "ok")
                    {
                        programSourceCodeFile.close ();
                        SendReturnStatusToHALshell (itos (nextPid), "error", "line " + itos (lineNo) + ": " + result);
                        nextPid ++;
                        return;
                    }
                    ram.SetP (currentProgramTextAddress);
                }
            }
            else if (!functionSeen)
            {
                currentProgramTextAddress = ram.GetP ();
                address = GetGlobalSymbolAddress (symbol);
                if (address == -1)
                {
                    programSourceCodeFile.close ();
                    SendReturnStatusToHALshell (itos (nextPid), "error", "line " + itos (lineNo) + ": " + symbol + " is an undeclared symbol");
                    nextPid ++;
                    return;
                }
                if (value == "integer" ||
                    value == "float" ||
                    value == "string" ||
                    value == "input" ||
                    value == "output")
                {
                    contents1.symbol = symbol;
                    contents1.value = value;
                    result = AssignTypeToGlobalSymbol (contents1);
                    if (result != "ok")
                    {
                        programSourceCodeFile.close ();
                        SendReturnStatusToHALshell (itos (nextPid), "error", "line " + itos (lineNo) + ": " + result);
                        nextPid ++;
                        return;
                    }
                    if (arraySize.length () > 0)
                    {
                        if (IsInteger (arraySize))
                        {
                            contents1.value = arraySize;
                            result = AllocateGlobalArray (contents1);
                            if (result != "ok")
                            {
                                programSourceCodeFile.close ();
                                SendReturnStatusToHALshell (itos (nextPid), "error", "line " + itos (lineNo) + ": " + result);
                                nextPid ++;
                                return;
                            }
                        }
                        else
                        {
                            address = GetGlobalSymbolAddress (arraySize);
                            if (address != -1)
                            {
                                ram.SetP (address);
                                contents2 = ram.Read (0);
                                ram.SetP (atoi (contents2.value.c_str ()));
                                contents2 = ram.Read (0);
                                contents1.value = contents2.value;
                                result = AllocateGlobalArray (contents1);
                                if (result != "ok")
                                {
                                    programSourceCodeFile.close ();
                                    SendReturnStatusToHALshell (itos (nextPid), "error", "line " + itos (lineNo) + ": " + result);
                                    nextPid ++;
                                    return;
                                }
                            }
                            else
                            {
                                programSourceCodeFile.close ();
                                SendReturnStatusToHALshell (itos (nextPid), "error", "line " + itos (lineNo) + ": " + arraySize + " is an undeclared symbol");
                                nextPid ++;
                                return;
                            }
                        }
                        arraySize = "";
                    }
                }
                else
                {
                    programSourceCodeFile.close ();
                    SendReturnStatusToHALshell (itos (nextPid), "error", "line " + itos (lineNo) + ": " + value + " is an unrecognized data type");
                    nextPid ++;
                    return;
                }
                ram.SetP (currentProgramTextAddress);
            }
            else
            {
                result = ValidateSymbol (symbol);
                if (result != "ok")
                {
                    result = ValidateValue (value);
                    if (result != "ok")
                    {
                        programSourceCodeFile.close ();
                        SendReturnStatusToHALshell (itos (nextPid), "error", "line " + itos (lineNo) + ": [" + symbol + " :" + value + "] contains an unrecognized symbol");
                        nextPid ++;
                        return;
                    }
                }
            }
            if (functionSeen)
            {
                ram.Write (symbol, value, 0);
                ram.IterateUp ();
            }
        }
        programSourceCodeFile >> symbol;
    }
    currentProgramTextAddress = ram.GetP ();
    SetKernelVariableValue ("END_ADDRESS", itos (currentProgramTextAddress - 1));
    ram.SetP (currentProgramTextAddress);

    programSourceCodeFile.close ();

    result = ProcessImageToFile (programName);
    if (result != "ok")
    {
        SendReturnStatusToHALshell (itos (nextPid), "error", result);
        nextPid ++;
    }
    else
    {
        SendReturnStatusToHALshell (itos (nextPid), "ok", programName + " compiled");
        nextPid ++;
    }

    return;
}

bool GetProcessImageTemplate ()
{
    ifstream processImageTemplateFile;
    char uselessCharacter;
    int address;
    char fieldSeparator;
    string symbol;
    string value;
    size_t foundLowerCaseAddress;
    size_t foundUpperCaseAddress;
    size_t foundInstructionPointer;

    processImageTemplateFile.open ("0_template");
    if (!processImageTemplateFile)
    {
        return false;
    }

    ram.Clear (0, 0);

    processImageTemplateFile >> uselessCharacter;
    while (processImageTemplateFile)
    {
        processImageTemplateFile >> uselessCharacter;
        processImageTemplateFile >> uselessCharacter;
        processImageTemplateFile >> address;
        ram.SetP (address);
        processImageTemplateFile >> fieldSeparator;
        processImageTemplateFile >> symbol;
        processImageTemplateFile.ignore (256, ':');
        foundLowerCaseAddress = symbol.find ("address");
        foundUpperCaseAddress = symbol.find ("ADDRESS");
        foundInstructionPointer = symbol.find ("INSTRUCTION_POINTER");
        if (foundLowerCaseAddress != string::npos ||
            foundUpperCaseAddress != string::npos ||
            foundInstructionPointer != string::npos)
        {
            processImageTemplateFile >> uselessCharacter;
            processImageTemplateFile >> uselessCharacter;
            processImageTemplateFile >> uselessCharacter;
        }
        getline (processImageTemplateFile, value);
        ram.Write (symbol, value, 0);
        processImageTemplateFile >> uselessCharacter;
    }

    processImageTemplateFile.close ();

    return true;
}

string Push (memoryCell contents1)
{
    int functionCallValuesStackStartAddress;
    int topFunctionCallValuesStackAddress;
    int segmentSize;
    memoryCell contents2;
    string result;

    functionCallValuesStackStartAddress = GetMemorySegmentBoundary ("FUNCTION_CALL_VALUES_STACK_START_ADDRESS", segmentSize);
    topFunctionCallValuesStackAddress = GetKernelVariableIntegerValue ("TOP_FUNCTION_CALL_VALUES_STACK_ADDRESS");
    ram.SetP (topFunctionCallValuesStackAddress);
    if (ram.GetP () == functionCallValuesStackStartAddress - segmentSize)
    {
        return ("function call values stack segmentation violation");
    }

    result = DetermineMemoryCellContentsForReturnAndPushCommands (contents1, contents2);
    if (result == "ok")
    {
        ram.SetP (GetKernelVariableIntegerValue ("TOP_FUNCTION_CALL_VALUES_STACK_ADDRESS"));
        ram.Push (contents2.symbol, contents2.value, 0);
        SetKernelVariableValue ("TOP_FUNCTION_CALL_VALUES_STACK_ADDRESS", itos (ram.GetP ()));
    }

    return (result);
}

string DetermineMemoryCellContentsForReturnAndPushCommands (memoryCell contents1, memoryCell &contents2)
{
    int address;
    int symbolAddress;
    int indexAddress;
    size_t foundArrayBracket;
    size_t foundArrayAddress;
    size_t foundFileAddress;
    string index;
    size_t indexStartPosition;
    size_t indexEndPosition;
    string result;

    // push/return string
    if (contents1.value [0] == '\'')
    {
        contents2.symbol = "string";
        if (contents1.value.length () > 1)
        {
            contents2.value = contents1.value.substr (1, contents2.value.length () - 1);
        }
        else
        {
            contents2.value = "";
        }
    }
    // push/return :@a or push/return :@a<?> (? is literal integer constant, variable name, or constant name)
    else if (contents1.value [0] == '@')
    {
        contents1.value = contents1.value.substr (1, contents1.value.length () - 1);
        foundArrayBracket = contents1.value.find ("<");
        // return :@a
        if (foundArrayBracket == string::npos)
        {
            symbolAddress = GetGlobalSymbolAddress (contents1.value);
            if (symbolAddress == -1)
            {
                return (contents1.value + " is an undeclared symbol");
            }
            ram.SetP (symbolAddress);
            contents1 = ram.Read (0);
            address = atoi (contents1.value.c_str ());
            ram.SetP (atoi (contents1.value.c_str ()));
            contents1 = ram.Read (0);
            contents2.symbol = contents1.symbol + "_address";
            contents2.value = itos (address);
        }
        // push/return :@a<?> (? is a literal integer constant, variable name, or constant name)
        else
        {
            indexStartPosition = foundArrayBracket + 1;
            indexEndPosition = contents1.value.find (">") - 1;
            index = contents1.value.substr (indexStartPosition, indexEndPosition - indexStartPosition + 1);
            contents1.value = contents1.value.substr (0, indexStartPosition - 1);
            symbolAddress = GetGlobalSymbolAddress (contents1.value);
            if (symbolAddress == -1)
            {
                return (contents1.value + " is an undeclared symbol");
            }
            ram.SetP (symbolAddress);
            contents1 = ram.Read (0);
            address = atoi (contents1.value.c_str ());
            ram.SetP (atoi (contents1.value.c_str ()));
            contents1 = ram.Read (0);
            foundArrayAddress = contents1.symbol.find ("array");
            contents2.symbol = contents1.symbol.substr (0, foundArrayAddress - 1) + "_address";
            // push/return :@a<9> (9 is any literal integer constant)
            if (IsInteger (index))
            {
                address = address + atoi (index.c_str ());
                contents2.value = itos (address);
            }
            // push/return :@a<i> (i is a variable name or constant name)
            else
            {
                indexAddress = GetGlobalSymbolAddress (index);
                if (indexAddress == -1)
                {
                    return (index + " is an undeclared symbol");
                }
                ram.SetP (indexAddress);
                contents1 = ram.Read (0);
                ram.SetP (atoi (contents1.value.c_str ()));
                contents1 = ram.Read (0);
                address = address + atoi (contents1.value.c_str ());
                contents2.value = itos (address);
            }
        }
    }
    else
    {
        foundArrayBracket = contents1.value.find ("<");
        // push/return :a<?> (? is a literal integer constant, variable name, or constant name)
        if (foundArrayBracket != string::npos)
        {
            indexStartPosition = foundArrayBracket + 1;
            indexEndPosition = contents1.value.find (">") - 1;
            index = contents1.value.substr (indexStartPosition, indexEndPosition - indexStartPosition + 1);
            contents1.value = contents1.value.substr (0, indexStartPosition - 1);
            symbolAddress = GetGlobalSymbolAddress (contents1.value);
            if (symbolAddress == -1)
            {
                return (contents1.value + " is an undeclared symbol");
            }
            ram.SetP (symbolAddress);
            contents1 = ram.Read (0);
            address = atoi (contents1.value.c_str ());
            ram.SetP (atoi (contents1.value.c_str ()));
            contents1 = ram.Read (0);
            contents2.symbol = GetDataType (contents1.symbol, result);
            if (result != "ok")
            {
                return (result);
            }
            // push/return :a<9> (9 is any literal integer constant)
            if (IsInteger (index))
            {
                ram.SetP (address + atoi (index.c_str ()));
            }
            // push/return :a<i> (i is a variable name or constant name)
            else
            {
                indexAddress = GetGlobalSymbolAddress (index);
                if (indexAddress == -1)
                {
                    return (index + " is an undeclared symbol");
                }
                ram.SetP (indexAddress);
                contents1 = ram.Read (0);
                ram.SetP (atoi (contents1.value.c_str ()));
                contents1 = ram.Read (0);
                ram.SetP (address + atoi (contents1.value.c_str ()));
            }
            contents1 = ram.Read (0);
            contents2.value = contents1.value;
        }
        // push/return :a or push/return :9 or push/return :9.9 or push/return :s
        else
        {
            symbolAddress = GetGlobalSymbolAddress (contents1.value);
            if (symbolAddress != -1)
            {
                ram.SetP (symbolAddress);
                contents1 = ram.Read (0);
                ram.SetP (atoi (contents1.value.c_str ()));
                contents1 = ram.Read (0);
                foundArrayAddress = contents1.symbol.find ("array");
                // push/return :a (where a is not an array name)
                if (foundArrayAddress == string::npos)
                {
                    foundFileAddress = contents1.symbol.find ("file");
                    if (foundFileAddress == string::npos)
                    {
                        contents2.symbol = GetDataType (contents1.symbol, result);
                        if (result != "ok")
                        {
                            return (result);
                        }
                        contents2.value = contents1.value;
                    }
                    // push/return :a (where a is a file)
                    else
                    {
                        contents2.symbol = GetDataType (contents1.symbol, result);
                        if (result != "ok")
                        {
                            return (result);
                        }
                        contents2.symbol = "global_" + contents2.symbol + "_address";
                        contents2.value = itos (ram.GetP ());
                    }
                }
                // push/return :a (where a is an array name)
                else
                {
                    foundArrayAddress = contents1.symbol.find ("0");
                    contents2.symbol = contents1.symbol.substr (0, foundArrayAddress - 1);
                    contents2.symbol = "global_" + contents2.symbol + "_address";
                    contents2.value = itos (ram.GetP ());
                }
            }
            // push/return :9 (9 is any integer)
            else if (IsInteger (contents1.value))
            {
                contents2.symbol = "integer";
                contents2.value = contents1.value;
            }
            // push/return :9.9 (9.9 is any float)
            else if (IsFloat (contents1.value))
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

    return ("ok");
}

string Set (memoryCell contents)
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
    string result;

    symbol = contents.value;

    ram.SetP (GetKernelVariableIntegerValue ("TOP_FUNCTION_CALL_VALUES_STACK_ADDRESS"));
    contents = ram.Pop (0);
    value = contents.value;
    SetKernelVariableValue ("TOP_FUNCTION_CALL_VALUES_STACK_ADDRESS", itos (ram.GetP ()));

    if (symbol [0] == '@')
    {
        symbol = symbol.substr (1, symbol.length () - 1);
    }
    foundArrayBracket = symbol.find ("<");
    if (foundArrayBracket != string::npos)
    {
        indexStartPosition = foundArrayBracket + 1;
        indexEndPosition = symbol.find (">") - 1;
        index = symbol.substr (indexStartPosition, indexEndPosition - indexStartPosition + 1);
        if (IsInteger (index))
        {
            symbol = symbol.substr (0, indexStartPosition - 1);
            addressOffset = atoi (index.c_str ());
        }
        else
        {
            address = GetGlobalSymbolAddress (index);
            if (address != -1)
            {
                ram.SetP (address);
                contents = ram.Read (0);
                symbol = symbol.substr (0, indexStartPosition - 1);
                ram.SetP (atoi (contents.value.c_str ()));
                contents = ram.Read (0);
                addressOffset = atoi (contents.value.c_str ());
            }
            else
            {
                return (index + " is an undeclared symbol");
            }
        }
    } 
    
    address = GetGlobalSymbolAddress (symbol);
    if (address != -1)
    {
        ram.SetP (address);
        contents = ram.Read (0);
        ram.SetP (atoi (contents.value.c_str ()) + addressOffset);
        contents = ram.Read (0);
        if (contents.symbol == "input_file" || contents.symbol == "output_file")
        {
            address = ram.GetP ();
            ram.SetP (address + 1);
            contents = ram.Read (0);
        }
        else
        {
            dataTypeCategory = GetDataTypeCategory (contents.symbol, result);
            if (result != "ok")
            {
                return (result);
            }
            if (dataTypeCategory == "constant" && contents.value != "?#@NULL_VALUE@#?")
            {
                return ("constant " + symbol + " has already been initialized");
            }
        }
    }
    else
    {
        return (symbol + " is an undeclared symbol");
    }

    ram.ReWrite ("", value, 0);

    return ("ok");
}

string ProcessImageToFile (string programName)
{
    ofstream executableFile;
    string executableName;
    memoryCell contents;
    int i;
    int globalSymbolsTableStartAddress;
    int globalSymbolsTableEndAddress;
    int functionCallStackEndAddress;
    int functionCallStackStartAddress;
    int segmentSize;
    string result;

    executableName = programName + "x";
    executableFile.open (executableName.c_str ());
    if (!executableFile)
    {
        result = "unable to create executable file for " + programName;
        return result;
    }

    globalSymbolsTableStartAddress = GetMemorySegmentBoundary ("GLOBAL_SYMBOLS_TABLE_START_ADDRESS", segmentSize);
    globalSymbolsTableEndAddress = globalSymbolsTableStartAddress + segmentSize;
    functionCallStackEndAddress = GetMemorySegmentBoundary ("FUNCTION_CALL_STACK_START_ADDRESS", segmentSize) + 1;
    functionCallStackStartAddress = functionCallStackEndAddress - segmentSize;

    for (i = 0; i < PARTITION_SIZE; i ++)
    {
        ram.SetP (i);
        contents = ram.Read (0);
        if (contents.symbol.length () > 0)
        {
            executableFile << "0d_";
            executableFile << i << ": ";
            executableFile << contents.symbol << " :";
            if (AddressField (contents.symbol, i,
                              globalSymbolsTableStartAddress,
                              globalSymbolsTableEndAddress,
                              functionCallStackStartAddress,
                              functionCallStackEndAddress))
            {
                executableFile << "0d_";
            }
            executableFile << contents.value << endl;
        }
    }

    executableFile.close ();

    return ("ok");
}

bool AddressField (string symbol, int i, int globalSymbolsTableStartAddress, int globalSymbolsTableEndAddress,
                   int functionCallStackStartAddress, int functionCallStackEndAddress)
{
    size_t foundLowerCaseAddress;
    size_t foundUpperCaseAddress;
    size_t foundInstructionPointer;

    foundLowerCaseAddress = symbol.find ("address");
    foundUpperCaseAddress = symbol.find ("ADDRESS");
    foundInstructionPointer = symbol.find ("INSTRUCTION_POINTER");
    if (foundLowerCaseAddress != string::npos ||
        foundUpperCaseAddress != string::npos ||
        foundInstructionPointer != string::npos)
    {
        return true;
    }
    else
    {
        if (symbol.length () > 0)
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

void SetKernelVariableValue (string kernelVariableDescription, string value)
{
    int startAddress;
    int segmentSize;
    memoryCell contents;

    startAddress = GetMemorySegmentBoundary ("KERNEL_SPACE_START_ADDRESS", segmentSize);
    startAddress = startAddress + segmentSize;
    ram.SetP (startAddress);
    ram.IterateDown ();
    while (1)
    {
        contents = ram.Read (0);
        if (contents.symbol == kernelVariableDescription)
        {
            break;
        }
        ram.IterateDown ();
    }
    ram.ReWrite ("", value, 0);

    return;
}

int GetMemorySegmentBoundary (string segmentStartAddressDescription, int &segmentSize)
{
    int startAddress;
    memoryCell contents;

    ram.ResetP ();
    ram.IterateUp ();
    while (1)
    {
        contents = ram.Read (0);
        if (contents.symbol == segmentStartAddressDescription)
        {
            break;
        }
        ram.IterateUp ();
    }
    startAddress = atoi (contents.value.c_str ());
    ram.IterateUp ();
    contents = ram.Read (0);
    segmentSize = atoi (contents.value.c_str ());

    return startAddress;
}

string AllocateGlobalSymbol (memoryCell contents, int currentProgramTextAddress)
{
    int globalValuesTableStartAddress;
    int lastGlobalValuesTableAddress;
    int globalSymbolsTableStartAddress;
    int lastGlobalSymbolsTableAddress;
    int segmentSize;

    globalValuesTableStartAddress = GetMemorySegmentBoundary ("GLOBAL_VALUES_TABLE_START_ADDRESS", segmentSize);
    lastGlobalValuesTableAddress = GetKernelVariableIntegerValue ("LAST_GLOBAL_VALUES_TABLE_ADDRESS");
    ram.SetP (lastGlobalValuesTableAddress);
    ram.IterateUp ();
    lastGlobalValuesTableAddress = ram.GetP ();
    if (lastGlobalValuesTableAddress == globalValuesTableStartAddress + segmentSize)
    {
        return ("size of global values table exceeds segment size");
    }
    if (contents.symbol == "function" ||
        contents.symbol == "label")
    {
        ram.Write ("constant_address", itos (currentProgramTextAddress), 0);
    }
    else if (contents.symbol == "constant")
    {
        ram.Write ("constant", "undefined_type", 0);
    }
    else if (contents.symbol == "variable")
    {
        ram.Write ("variable", "undefined_type", 0);
    }
    else if (contents.symbol == "file")
    {
        if (contents.value == "keyboard")
        {
            ram.Write ("input_" + contents.symbol, "closed", 0);
        }
        else if (contents.value == "display")
        {
            ram.Write ("output_" + contents.symbol, "closed", 0);
        }
        else
        {
            ram.Write (contents.symbol, "", 0);
        }
    }
    SetKernelVariableValue ("LAST_GLOBAL_VALUES_TABLE_ADDRESS", itos (lastGlobalValuesTableAddress));
    globalSymbolsTableStartAddress = GetMemorySegmentBoundary ("GLOBAL_SYMBOLS_TABLE_START_ADDRESS", segmentSize);
    lastGlobalSymbolsTableAddress = GetKernelVariableIntegerValue ("LAST_GLOBAL_SYMBOLS_TABLE_ADDRESS");
    ram.SetP (lastGlobalSymbolsTableAddress);
    ram.IterateUp ();
    lastGlobalSymbolsTableAddress = ram.GetP ();
    if (lastGlobalSymbolsTableAddress == globalSymbolsTableStartAddress + segmentSize)
    {
        return ("size of global symbols table exceeds segment size");
    }
    ram.Write (contents.value, itos (lastGlobalValuesTableAddress), 0);
    SetKernelVariableValue ("LAST_GLOBAL_SYMBOLS_TABLE_ADDRESS", itos (lastGlobalSymbolsTableAddress));

    if (contents.symbol == "file")
    {
        if (contents.value != "keyboard" && contents.value != "display")
        {
            globalValuesTableStartAddress = GetMemorySegmentBoundary ("GLOBAL_VALUES_TABLE_START_ADDRESS", segmentSize);
            lastGlobalValuesTableAddress = GetKernelVariableIntegerValue ("LAST_GLOBAL_VALUES_TABLE_ADDRESS");
            ram.SetP (lastGlobalValuesTableAddress);
            ram.IterateUp ();
            lastGlobalValuesTableAddress = ram.GetP ();
            if (lastGlobalValuesTableAddress == globalValuesTableStartAddress + segmentSize)
            {
                return ("size of global values table exceeds segment size");
            }
            ram.Write ("file_name", "", 0);
            SetKernelVariableValue ("LAST_GLOBAL_VALUES_TABLE_ADDRESS", itos (lastGlobalValuesTableAddress));
        }
    }

    return ("ok");
}

string AssignTypeToGlobalSymbol (memoryCell contents)
{
    int address;
    string symbol;
    string type;

    symbol = contents.symbol;
    type = contents.value;

    address = GetGlobalSymbolAddress (symbol);
    if (address != -1)
    {
        ram.SetP (address);
        contents = ram.Read (0);
        ram.SetP (atoi (contents.value.c_str ()));
        contents = ram.Read (0);
        if (contents.symbol == "constant")
        {
            if (type == "integer")
            {
                ram.ReWrite (contents.symbol + "_" + type, "?#@NULL_VALUE@#?", 0);
            }
            else if (type == "float")
            {
                ram.ReWrite (contents.symbol + "_" + type, "?#@NULL_VALUE@#?", 0);
            }
            else if (type == "string")
            {
                ram.ReWrite (contents.symbol + "_" + type, "?#@NULL_VALUE@#?", 0);
            }
            else
            {
                return ("unrecognized data type " + type + " for global symbol " + symbol);
            }
        }
        else if (contents.symbol == "variable")
        {
            if (type == "integer")
            {
                ram.ReWrite (contents.symbol + "_" + type, "0", 0);
            }
            else if (type == "float")
            {
                ram.ReWrite (contents.symbol + "_" + type, "0.0", 0);
            }
            else if (type == "string")
            {
                ram.ReWrite (contents.symbol + "_" + type, "", 0);
            }
            else
            {
                return ("unrecognized data type " + type + " for global symbol " + symbol);
            }
        }
        else if (contents.symbol == "file")
        {
            if (type == "input")
            {
                ram.ReWrite (type + "_" + contents.symbol, "closed", 0);
            }
            else if (type == "output")
            {
                ram.ReWrite (type + "_" + contents.symbol, "closed", 0);
            }
            else
            {
                return ("unrecognized data type " + type + " for global symbol " + symbol);
            }
            ram.IterateUp ();
            contents = ram.Read (0);
            ram.ReWrite (type + "_" + contents.symbol, "", 0);
        }
    }
    else
    {
        return ("symbol " + symbol + " not found in global symbol table");
    }

    return ("ok");
}

string AllocateGlobalArray (memoryCell contents)
{
    int globalValuesTableStartAddress;
    int lastGlobalValuesTableAddress;
    int segmentSize;
    int noOfArrayElements = atoi (contents.value.c_str ());
    string symbol;
    int i;

    globalValuesTableStartAddress = GetMemorySegmentBoundary ("GLOBAL_VALUES_TABLE_START_ADDRESS", segmentSize);
    lastGlobalValuesTableAddress = GetKernelVariableIntegerValue ("LAST_GLOBAL_VALUES_TABLE_ADDRESS");
    ram.SetP (lastGlobalValuesTableAddress);
    if (lastGlobalValuesTableAddress == globalValuesTableStartAddress + noOfArrayElements + segmentSize)
    {
        return ("size of global values table exceeds segment size");
    }
    contents = ram.Read (0);
    symbol = contents.symbol;
    contents.symbol = contents.symbol + "_array_0";
    ram.ReWrite (contents.symbol, contents.value, 0);

    for (i = 1; i < noOfArrayElements; i ++)
    {
        ram.SetP (GetKernelVariableIntegerValue ("LAST_GLOBAL_VALUES_TABLE_ADDRESS"));
        ram.IterateUp ();
        contents.symbol = symbol + "_array_" + itos (i);
        ram.Write (contents.symbol, contents.value, 0);
        SetKernelVariableValue ("LAST_GLOBAL_VALUES_TABLE_ADDRESS", itos (ram.GetP ()));
    }

    return ("ok");
}

int GetKernelVariableIntegerValue (string kernelVariableDescription)
{
    int startAddress;
    int segmentSize;
    memoryCell contents;
    int kernelVariableValue;

    startAddress = GetMemorySegmentBoundary ("KERNEL_SPACE_START_ADDRESS", segmentSize);
    startAddress = startAddress + segmentSize;
    ram.SetP (startAddress);
    ram.IterateDown ();
    while (1)
    {
        contents = ram.Read (0);
        if (contents.symbol == kernelVariableDescription)
        {
            break;
        }
        ram.IterateDown ();
    }
    kernelVariableValue = atoi (contents.value.c_str ());

    return (kernelVariableValue);
}

int GetGlobalSymbolAddress (string symbol)
{
    int globalSymbolsTableStartAddress;
    int lastGlobalSymbolsTableAddress;
    int segmentSize;
    memoryCell contents;

    globalSymbolsTableStartAddress = GetMemorySegmentBoundary ("GLOBAL_SYMBOLS_TABLE_START_ADDRESS", segmentSize);
    lastGlobalSymbolsTableAddress = GetKernelVariableIntegerValue ("LAST_GLOBAL_SYMBOLS_TABLE_ADDRESS");
    ram.SetP (globalSymbolsTableStartAddress);
    contents = ram.Read (0);
    while (ram.GetP () <= lastGlobalSymbolsTableAddress)
    {
        if (contents.symbol == symbol)
        {
            return ram.GetP ();
        }
        ram.IterateUp ();
        contents = ram.Read (0);
    }

    return -1;
}

string GetDataType (string dataType, string &result)
{
    size_t foundDataType;

    foundDataType = dataType.find ("integer");
    if (foundDataType != string::npos)
    {
        result = "ok";
        return ("integer");
    }

    foundDataType = dataType.find ("float");
    if (foundDataType != string::npos)
    {
        result = "ok";
        return ("float");
    }

    foundDataType = dataType.find ("string");
    if (foundDataType != string::npos)
    {
        result = "ok";
        return ("string");
    }

    result = dataType + " is unrecognized or invalid data type ";
    return ("");
}

string GetDataTypeCategory (string dataTypeCategory, string &result)
{
    size_t foundDataTypeCategory;

    foundDataTypeCategory = dataTypeCategory.find ("constant");
    if (foundDataTypeCategory != string::npos)
    {
        result = "ok";
        return ("constant");
    }

    foundDataTypeCategory = dataTypeCategory.find ("variable");
    if (foundDataTypeCategory != string::npos)
    {
        result = "ok";
        return ("variable");
    }

    result = dataTypeCategory + " is unrecognized or invalid data type category";
    return ("");
}

string ValidateSymbol (string symbol)
{
    if (symbol == "comment" || symbol == "call" || symbol == "return" || symbol == "push" ||
        symbol == "pop" || symbol == "set" || symbol == "compare" || symbol == "add" ||
        symbol == "subtract" || symbol == "multiply" || symbol == "divide" || symbol == "modulo" ||
        symbol == "join" || symbol == "variable" || symbol == "constant" || symbol == "reference" ||
        symbol == "jump" || symbol == "jumpless" || symbol == "jumpequal" || symbol == "jumpgreater" ||
        symbol == "file" || symbol == "open" || symbol == "read" || symbol == "write" ||
        symbol == "newline" || symbol == "close" || symbol == "coresnapshot")
    {
        return "ok";
    }

    return "";
}

string ValidateValue (string value)
{
    if (value == "integer" || value == "float" || value == "string" || value == "input" || value == "output")
    {
        return "ok";
    }

    return "";
}

bool IsInteger (string value)
{
    int i;

    if (value.length () == 0)
    {
        return false;
    }

    for (i = 0; i < value.length (); i ++)
    {
        if (i == 0 && value [i] == '-')
        {
            continue;
        }
        else if (!isdigit (value [i]))
        {
            return false;
        }
    }

    return true;
}

bool IsFloat (string value)
{
    int i;
    bool decimalPointSeen = false;

    if (value.length () == 0)
    {
        return false;
    }

    for (i = 0; i < value.length (); i ++)
    {
        if (i == 0 && value [i] == '-')
        {
            continue;
        }
        else if (value [i] == '.')
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
        else if (!isdigit (value [i]))
        {
            return false;
        }
    }

    return true;
}
