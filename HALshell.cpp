//
// HALshell.cpp
//
// Copyright (c) 2019 Robert J. Hilderman.
// All Rights Reserved.
//

#include "HALshell.h"
#include "HALosHALshellFileIO.h"




/*******************
 *                 *
 *    The Shell    *
 *                 *
 *******************/


#pragma region The_Shell


void HALshell ()
{
    // Initialize the shell info

    shellInfo.nextCommandNum = 1;

    // Shell name
    ifstream shellname("shellname");
    if (!shellname)
    {
        cout << "HALshell: could not open shellname, using default name" << endl;
        shellInfo.shellName = "HALshell";
    }
    else
    {
        getline(shellname, shellInfo.shellName);
    }
    shellname.close();

    // Shell terminator
    ifstream terminator("terminator");
    if (!terminator)
    {
        cout << "HALshell: could not open terminator, using default terminator" << endl;
        shellInfo.terminator = ">";
    }
    else
    {
        getline(terminator, shellInfo.terminator);
    }
    terminator.close();

    // History size
    ifstream historysize("historysize");
    if (!historysize)
    {
        cout << "HALshell: could not open historysize, using default history size" << endl;
        shellInfo.historySize = 10;
    }
    else
    {
        string historySizeString;
        getline(historysize, historySizeString);
        shellInfo.historySize = atoi(historySizeString.c_str());
    }
    historysize.close();

    // New name size
    ifstream newnamesize("newnamesize");
    if (!newnamesize)
    {
        cout << "HALshell: could not open newnamesize, using default new name size" << endl;
        shellInfo.newNameSize = 10;
    }
    else
    {
        string newNameSizeString;
        getline(newnamesize, newNameSizeString);
        shellInfo.newNameSize = atoi(newNameSizeString.c_str());
    }
    newnamesize.close();


    CommandHandling();

    return;
}

/*
    Function: CommandHandling
    Input:
        command (optional) - entered command to be handled
    Output:
        void
    Purpose:
        Handle the passing of commands. If no command is passed then it will be
        fetched from the command line. If one is passed, only execute that one command
        then exit.
*/
void CommandHandling(string command)
{
    string tokens [MAX_COMMAND_LINE_ARGUMENTS];
    string commandLine = "";
    int tokenCount;
    bool infinite = true;

    do
    {
        int newTokenCount = 0;

        // Standard operation, infinite loop
        if (command == "")
        {
            tokenCount = GetCommand (commandLine, tokens);
        }
        // Run only once, called by history re-execution command
        else
        {
            infinite = false;
            commandLine = command;
            tokenCount = TokenizeCommandLine(tokens, command);
        }

        string newNameTokens [MAX_COMMAND_LINE_ARGUMENTS];

        // Don't do substitution if command is setnewname
        if (tokens[0] != "setnewname")
        {
            newTokenCount = SubstituteNewNames(tokens, newNameTokens, tokenCount);
        }

        // -1 If aliasing produces a command that is too long
        if (newTokenCount < 0)
        {
            cout << "HALshell: alias exceeds maximum number of command line arguments"
                << endl;
        }
        // Substitution
        else if (newTokenCount > 0)
        {
            string newCommandLine = AccumulateTokens(newNameTokens, newTokenCount, 0);

            // Do not add the re-execution command to the history
            if (newCommandLine.at(0) != '!')
            {
                // Add the command to the history
                shellInfo.history.insert(shellInfo.history.begin(), newCommandLine);
                if (shellInfo.history.size() > shellInfo.historySize)
                {
                    shellInfo.history.pop_back();
                }
            }

            ProcessCommand (newNameTokens, newTokenCount);
        }
        // No Substitution
        else
        {
            // Do not add the re-execution command to the history
            if (commandLine.at(0) != '!')
            {
                // Add the command to the history
                shellInfo.history.insert(shellInfo.history.begin(), commandLine);
                if (shellInfo.history.size() > shellInfo.historySize)
                {
                    shellInfo.history.pop_back();
                }
            }

            ProcessCommand (tokens, tokenCount);
        }
    } while (infinite);

    return;
}

/*
    Function: SubstituteNewNames
    Input:
        tokens - the tokens to substitute new names for
        oldTokenCount - the count of tokens in the tokens array
    Output:
        newNameTokens - the tokens that will have the substituted names
        int - the count of tokens in the substituted token array
    Purpose:
        Replace any tokens in the tokens array that match a name
        in the shell's new name list.
*/
int SubstituteNewNames (string tokens[], string newNameTokens[],
                        int oldTokenCount)
{
    int newTokenCount = 0;
    int previousTokenCount = oldTokenCount;
    bool isAlias = false;
    vector<string> newNameTokensVector(tokens, tokens + oldTokenCount);
    
    // Iterate over the passed tokens
    for (int i = 0; i < newNameTokensVector.size(); /**/)
    {
        isAlias = false;

        // Iterate over the shell's new name list
        for (int j = 0; j < shellInfo.newNames.size(); ++j)
        {
            // Check the token against the new name alias
            if (newNameTokensVector.at(i) == shellInfo.newNames.at(j).at(0))
            {
                isAlias = true;

                // Check that the aliasing will no go over the maximum number of args
                // oldTokenCount - 1 for the replaced token
                // shellInfo.newNames.at(j).size() - 1 for the alias token
                newTokenCount = ((previousTokenCount - 1)
                                + (shellInfo.newNames.at(j).size() - 1));
                if (newTokenCount > MAX_COMMAND_LINE_ARGUMENTS)
                {
                    return -1;
                }
                previousTokenCount = newTokenCount;

                // Copy all but the keyword the alias replaces
                vector<string> alias;
                for (int k = 1; k < shellInfo.newNames.at(j).size(); ++k)
                {
                    alias.push_back(shellInfo.newNames.at(j).at(k));
                }
                
                // Replace the alias
                newNameTokensVector.erase(newNameTokensVector.begin() + i);
                newNameTokensVector.insert(newNameTokensVector.begin() + i,
                    alias.begin(), alias.end());
            }
        }

        // Move to the next token only if it was not an alias
        // Allows for multiple substitutions of the same element
        if (!isAlias) ++i;
    }

    copy (newNameTokensVector.begin(), newNameTokensVector.end(), newNameTokens);
    return newTokenCount;
}

int GetCommand (string & commandLine, string tokens [])
{
    //string commandLine;
    bool commandEntered;
    int tokenCount;

    do
    {
        BlockSignals ("HALshell");
        // Print the shell info
        cout << shellInfo.shellName << "[" << shellInfo.nextCommandNum << "]" << shellInfo.terminator;
        while (1)
        {
            getline (cin, commandLine);
            commandEntered = CheckForCommand ();
            if (commandEntered)
            {
                break;
            }
        }
        UnblockSignals ("HALshell");
    } while (commandLine.length () == 0);

    // Increment number of commands entered
    shellInfo.nextCommandNum++;

    tokenCount = TokenizeCommandLine (tokens, commandLine);

    return tokenCount;
}

int TokenizeCommandLine (string tokens [], string commandLine)
{
    char *token [MAX_COMMAND_LINE_ARGUMENTS];
    char *workCommandLine = new char [commandLine.length () + 1];
    int i;
    int tokenCount;

    for (i = 0; i < MAX_COMMAND_LINE_ARGUMENTS; i ++)
    {
        tokens [i] = "";
    }
    strcpy (workCommandLine, commandLine.c_str ());
    i = 0;
    if ((token [i] = strtok (workCommandLine, " ")) != NULL)
    {
        i ++;
        while ((token [i] = strtok (NULL, " ")) != NULL)
        {
            i ++;
        }
    }
    tokenCount = i;

    for (i = 0; i < tokenCount; i ++)
    {
        tokens [i] = token [i];
    }

    delete [] workCommandLine;

    return tokenCount;
}

void ProcessCommand (string tokens [], int tokenCount)
{
    // Built-in commands
    if (tokens [0] == "." || tokens [0] == "..")
    {
        BadCommand ();
        return;
    }
    
    if (tokens [0] == "about")
    {
        About (tokenCount);
        return;
    }
    else if (tokens [0] == "result")
    {
        Result (tokenCount);
        return;
    }
    else if (tokens [0] == "shutdown" || tokens [0] == "restart")
    {
        ShutdownAndRestart (tokens, tokenCount);
        // if no error, then never returns
        return;
    }

    // Custom commands
    else if (tokens [0] == "setshellname")
    {
        SetShellName (tokens, tokenCount);
        return;
    }
    else if (tokens [0] == "setterminator")
    {
        SetTerminator (tokens, tokenCount);
        return;
    }
    else if (tokens [0] == "sethistorysize")
    {
        SetHistorySize (tokens, tokenCount);
        return;
    }
    else if (tokens [0] == "showhistorysize")
    {
        ShowHistorySize (tokens, tokenCount);
        return;
    }
    else if (tokens[0] == "showhistory")
    {
        ShowHistory (tokens, tokenCount);
        return;
    }
    else if (tokens[0] == "!")
    {
        ExecuteHistory (tokens, tokenCount);
        return;
    }
    else if (tokens[0] == "setnewnamesize")
    {
        SetNewNameSize (tokens, tokenCount);
        return;
    }
    else if (tokens[0] == "shownewnamesize")
    {
        ShowNewNameSize (tokens, tokenCount);
        return;
    }
    else if (tokens[0] == "setnewname")
    {
        SetNewName (tokens, tokenCount);
        return;
    }
    else if (tokens[0] == "shownewnames")
    {
        ShowNewNames (tokens, tokenCount);
        return;
    }
    else if (tokens[0] == "writenewnames")
    {
        WriteNewNames (tokens, tokenCount);
        return;
    }
    else if (tokens[0] == "readnewnames")
    {
        ReadNewNames (tokens, tokenCount);
        return;
    }
    else if (tokens[0] == "restoredefaults")
    {
        RestoreDefaults (tokens, tokenCount);
        return;
    }

    SendCommandLineToHALos (tokens, tokenCount);
    Wait ();
    GetMessageFromHALos ();
    PrintReturnStatus ();

    return;
}

void PrintReturnStatus ()
{
    if (returnMessage.length () > 0 && returnMessage != "ok" && returnMessage.substr (0, 15) != "creation time =")
    {
        cout << "HALshell: ";
        if (returnPid != "")
        {
            cout << "PID = [" << returnPid << "] ";
        }
        if (returnValue != "")
        {
            cout << "RETURN_VALUE = [" << returnValue << "] ";
        }
        if (returnMessage != "")
        {
            cout << "MESSAGE = [" << returnMessage << "]";
        }
        cout << endl;
        returnPid = "";
    }

    return;
}

#pragma endregion The_Shell




/*********************************
 *                               *
 *    The Communication Media    *
 *                               *
 *********************************/


#pragma region The_Communication_Media


void GetMessageFromHALos ()
{
    ifstream halOsMessageFile;

    halOsMessageFile.open ("HALosToHALshell");
    if (!halOsMessageFile)
    {
        cout << "HALshell: connection to HALos failed" << endl;
        exit (1);
    }

    getline (halOsMessageFile, returnPid);
    getline (halOsMessageFile, returnValue);
    getline (halOsMessageFile, returnMessage);

    if (!halOsMessageFile)
    {
        cout << "HALshell: message not received from HALos" << endl;
        return;
    }

    halOsMessageFile.close ();

    return;
}

void SendCommandLineToHALos (string tokens [], int tokenCount)
{
    ofstream commandLineFile;
    int i;
    union sigval dummyValue;

    commandLineFile.open ("HALshellToHALos");
    if (!commandLineFile)
    {
        cout << "HALshell: unable to initialize command line buffer" << endl;
        exit (1);
    }

    for (i = 0; i < tokenCount; i ++)
    {
        commandLineFile << tokens [i] << endl;
    }
    commandLineFile.close ();
    if (sigqueue (HALosPid, SIGRTMIN, dummyValue) == -1)
    {
        cout << "HALshell: command line signal not sent to HALos" << endl;
        exit (1);
    }

    return;
}

#pragma endregion The_Communication_Media




/***************************
 *                         *
 *    Built-in Commands    *
 *                         *
 ***************************/


#pragma region Built-in_Commands


void About (int tokenCount)
{
    if (tokenCount > 1)
    {
        cout << "HALshell: about does not require any arguments" << endl;
        return;
    }

    cout << endl;
    cout << "*********************************************" << endl;
    cout << "*               HALos v5.4.6                *" << endl;
    cout << "*                                           *" << endl;
    cout << "*  Copyright (c) 2019 Robert J. Hilderman.  *" << endl;
    cout << "*           All Rights Reserved.            *" << endl;
    cout << "*********************************************" << endl;
    cout << endl;

    return;
}

void BadCommand ()
{
    cout << "HALshell: MESSAGE = [command not executable]" << endl;

    return;
}

void Result (int tokenCount)
{
    if (tokenCount > 1)
    {
        cout << "HALshell: result does not require any arguments" << endl;
        return;
    }

    if (returnPid != "")
    {
        cout << "HALshell: ";
        cout << "PID = [" << returnPid << "] ";
        if (returnValue != "")
        {
            cout << "RETURN_VALUE = [" << returnValue << "] ";
        }
        if (returnMessage != "")
        {
            cout << "MESSAGE = [" << returnMessage << "]";
        }
        cout << endl;
        returnPid = "";
    }
    
    // Print the return results of background programs
    // if the file exists
    if (access(HALosHALshellFileIO::fileName, 0) == 0)
    {
        vector<string> backgroundReturnResults = HALosHALshellFileIO::ReadFile();

        // Assume every result is three lines
        for (vector<string>::iterator it = backgroundReturnResults.begin();
             it != backgroundReturnResults.end();
             /*++it*/)
        {
            cout << "HALshell: ";
            cout << "PID = [" << *it << "] ";
            ++it;
            if (*it != "")
            {
                cout << "RETURN_VALUE = [" << *it << "] ";
            }
            ++it;
            if (*it != "")
            {
                cout << "MESSAGE = [" << *it << "]";
            }
            ++it;
            cout << endl;
        }

        // Remove the file
        if (remove (HALosHALshellFileIO::fileName) != 0)
        {
            cout << "HALshell: cannot remove HALosHALshell return status file" << endl;
            exit (1);
        }
    }
    
    return;
}

void ShutdownAndRestart (string tokens [], int tokenCount)
{
    if (tokenCount > 1)
    {
        cout << "HALshell: " << tokens [0] << " does not require any arguments" << endl;
        return;
    }

    cout << endl;
    cout << "HALshell: terminating ..." << endl;
    system ("HALshellCleanup");
    usleep (SLEEP_DELAY);
    SendCommandLineToHALos (tokens, tokenCount);

    exit (0);
}

#pragma endregion Built-in_Commands




/*************************
 *                       *
 *    Custom Commands    *
 *                       *
 *************************/


#pragma region Custom_Commands


/*
    Function: SetShellName
    Input:
        tokens - the tokens that have been passed from the command line
        tokenCount - the count of the tokens passed
    Output:
        global shellInfo updated
        file shellname updated
    Purpose:
        Set the name of the shell and write it to file
*/
void SetShellName (string tokens [], int tokenCount)
{
    // Check that a name has been provided
    if (tokenCount < 2)
    {
        cout << "HALshell: " << tokens[0] << " requires at least one argument" << endl;
        return;
    }

    string input = AccumulateTokens(tokens, tokenCount);

    // Set the new shell name
    shellInfo.shellName = input;

    // Write the new name to file
    ofstream shellname;
    shellname.open("shellname");
    if (!shellname)
    {
        cout << "HALshell: failed to open shellname" << endl;
        return;
    }

    shellname << shellInfo.shellName;
    shellname.close();

    return;
}

/*
    Function: SetTerminator
    Input:
        tokens - the tokens that have been passed from the command line
        tokenCount - the count of the tokens passed
    Output:
        global shellInfo updated
        file terminator updated
    Purpose:
        Set the shell's terminator
*/
void SetTerminator (string tokens[], int tokenCount)
{
    // Check that a terminator has been provided
    if (tokenCount < 2)
    {
        cout << "HALshell: " << tokens[0] << " requires at least one argument" << endl;
        return;
    }

    string input = AccumulateTokens(tokens, tokenCount);
    input += " ";

    // Set the new terminator
    shellInfo.terminator = input;

    // Write the new terminator to file
    ofstream terminator;
    terminator.open("terminator");
    if (!terminator)
    {
        cout << "HALshell: failed to open terminator" << endl;
        return;
    }
    
    terminator << shellInfo.terminator;
    terminator.close();

    return;
}

/*
    Function: SetHistorySize
    Input:
        tokens - the tokens that have been passed from the command line
        tokenCount - the count of the tokens passed
    Output:
        global shellInfo updated
        file historysize updated
    Purpose:
        Set the maximum length of the shell's history list
*/
void SetHistorySize (string tokens[], int tokenCount)
{
    // Check there is only one argument
    if (tokenCount != 2)
    {
        cout << "HALshell: " << tokens[0] << " requires one argument" << endl;
        return;
    }

    // Check that the argument is an integer
    if (!IsInteger(tokens[1]))
    {
        cout << "HALshell: " << tokens[0] << " only takes integers as an argument" << endl;
        return;
    }

    int newHistorySize = atoi(tokens[1].c_str());

    // Negative values arent permitted
    if (newHistorySize < 0)
    {
        cout << "HALshell: " << tokens[0] << " only allows positive integers" << endl;
        return;
    }

    // Remove excess history items
    while (shellInfo.history.size() > newHistorySize)
    {
        shellInfo.history.pop_back();
    }

    // Set the new history size
    shellInfo.historySize = newHistorySize;

    // Write the history size to file
    ofstream historysize;
    historysize.open("historysize");
    if (!historysize)
    {
        cout << "HALshell: failed to open historysize" << endl;
        return;
    }

    historysize << shellInfo.historySize;
    historysize.close();

    return;
}

/*
    Function: ShowHistorySize
    Input:
        tokens - the tokens that have been passed from the command line
        tokenCount - the count of the tokens passed
    Output:
        print the history size
    Purpose:
        Prints the maximum size of the shell's history list
*/
void ShowHistorySize (string tokens[], int tokenCount)
{
    // Check there are no arguments
    if (tokenCount > 1)
    {
        cout << "HALshell: " << tokens [0] << " does not require any arguments" << endl;
        return;
    }

    // Print the history size
    cout << shellInfo.historySize << endl;

    return;
}

/*
    Function: ShowHistory
    Input:
        tokens - the tokens that have been passed from the command line
        tokenCount - the count of the tokens passed
    Output:
        print the command history
    Purpose:
        Prints the contents of the command history list
*/
void ShowHistory (string tokens[], int tokenCount)
{
    // Check there are no arguments
    if (tokenCount > 1)
    {
        cout << "HALshell: " << tokens [0] << " does not require any arguments" << endl;
        return;
    }

    // Print the history list
    {
        vector<string>::reverse_iterator rit;
        int i;
        for (rit = shellInfo.history.rbegin(),
             i = shellInfo.history.size();
             rit != shellInfo.history.rend();
             ++rit, --i)
        {
            cout << setw(4) << left << itos(i) + ". " << *rit << endl;
        }
    }

    return;
}

/*
    Function: ExecuteHistory
    Input:
        tokens - the tokens that have been passed from the command line
        tokenCount - the count of the tokens passed
    Output:
        void
    Purpose:
        Re-executes the nth most recent command in the history list
*/
void ExecuteHistory (string tokens[], int tokenCount)
{
    // Check there is at most one argument
    if (tokenCount > 2)
    {
        cout << "HALshell: " << tokens[0] << " requires at most one argument" << endl;
        return;
    }

    // Most recent command
    if (tokenCount == 1)
    {
        CommandHandling(shellInfo.history.front());
    }
    // Nth command
    else
    {
        // Check that the argument is an integer
        if (!IsInteger(tokens[1]))
        {
            cout << "HALshell: " << tokens[0] << " only accepts integers as an arguments" << endl;
            return;
        }

        int commandNum = atoi(tokens[1].c_str());

        // Validate the entered value
        if (commandNum > shellInfo.history.size() ||
            commandNum > shellInfo.historySize ||
            commandNum < 1)
        {
            cout << "HALshell: " << commandNum
                << " is not a valid history number" << endl;
            return;
        }

        CommandHandling(shellInfo.history.at(commandNum - 1));
    }

    return;
}

/*
    Function: SetNewNameSize
    Input:
        tokens - the tokens that have been passed from the command line
        tokenCount - the count of the tokens passed
    Output:
        global shellInfo updated
        file newnamesize updated
    Purpose:
        Changes the limit of aliases to the specified amount
*/
void SetNewNameSize (string tokens[], int tokenCount)
{
    // Check there is only one argument
    if (tokenCount != 2)
    {
        cout << "HALshell: " << tokens[0] << " requires one argument" << endl;
        return;
    }

    // Check that the argument is an integer
    if (!IsInteger(tokens[1]))
    {
        cout << "HALshell: " << tokens[0] << " only takes integers as an argument" << endl;
        return;
    }

    int newNewNameSize = atoi(tokens[1].c_str());

    // Negative values arent permitted
    if (newNewNameSize < 0)
    {
        cout << "HALshell: " << tokens[0] << " only allows positive integers" << endl;
        return;
    }

    // Remove excess new names
    while (shellInfo.newNames.size() > newNewNameSize)
    {
        shellInfo.newNames.pop_back();
    }

    // Set the new history size
    shellInfo.newNameSize = newNewNameSize;

    // Write the history size to file
    ofstream newnamesize;
    newnamesize.open("newnamesize");
    if (!newnamesize)
    {
        cout << "HALshell: failed to open newnamesize" << endl;
        return;
    }

    newnamesize << shellInfo.newNameSize;
    newnamesize.close();

    return;
}

/*
    Function: ShowNewNameSize
    Input:
        tokens - the tokens that have been passed from the command line
        tokenCount - the count of the tokens passed
    Output:
        print alias limit
    Purpose:
        Prints the current limit on the number of aliases
*/
void ShowNewNameSize (string tokens[], int tokenCount)
{
    // Check there are no arguments
    if (tokenCount > 1)
    {
        cout << "HALshell: " << tokens [0] << " does not require any arguments" << endl;
        return;
    }

    // Print the new name size
    cout << shellInfo.newNameSize << endl;

    return;
}

/*
    Function: SetNewName
    Input:
        tokens - the tokens that have been passed from the command line
        tokenCount - the count of the tokens passed
    Output:
        global shellInfo updated
    Purpose:
        Add a new alias to the shell's new names list
*/
void SetNewName (string tokens[], int tokenCount)
{
    // Check there are at least 2 arguments
    if (tokenCount < 3)
    {
        cout << "HALshell: " << tokens [0] << " requires at least two arguments" << endl;
        return;
    }

    // Check the alias is different than the aliased command
    if (tokenCount == 3 &&
        tokens[1] == tokens[2])
    {
        cout << "HALshell: " << tokens[0]
            << " requires the alias and aliased commands be different" << endl;
        return;
    }

    // Collect the relevant tokens into a single container
    vector<string> alias;
    for (int i = 1; i < tokenCount; ++i)
    {
        alias.push_back(tokens[i]);
    }
    
    // Remove aliases of the same name
    for (vector< vector<string> >::iterator it = shellInfo.newNames.begin();
         it != shellInfo.newNames.end();)
    {
        if (it->at(0) == alias.at(0))
        {
            it = shellInfo.newNames.erase(it);
        }
        else
        {
            ++it;
        }
    }
    
    // Add the new alias
    shellInfo.newNames.push_back(alias);

    // Remove excess aliases
    if (shellInfo.newNames.size() > shellInfo.newNameSize)
    {
        shellInfo.newNames.pop_back();
    }

    return;
}

/*
    Function: ShowNewNames
    Input:
        tokens - the tokens that have been passed from the command line
        tokenCount - the count of the tokens passed
    Output:
        print aliases
    Purpose:
        Print the list of the shell's currently loaded aliases
*/
void ShowNewNames (string tokens[], int tokenCount)
{
    // Check there are no arguments
    if (tokenCount > 1)
    {
        cout << "HALshell: " << tokens [0] << " requires no arguments" << endl;
        return;
    }

    // Print the new names

    // Iterate over the new name list
    for (vector< vector<string> >::iterator oit = shellInfo.newNames.begin();
         oit != shellInfo.newNames.end();
         ++oit)
    {
        // Iterate over each entries' tokens
        for (vector<string>::iterator iit = oit->begin();
             iit != oit->end();
             ++iit)
        {
            cout << *iit << " ";
        }
        cout << endl;
    }
    
    return;
}

/*
    Function: WriteNewNames
    Input:
        tokens - the tokens that have been passed from the command line
        tokenCount - the count of the tokens passed
    Output:
        write to given file
    Purpose:
        Print the shell's current list of loaded aliases to
        a file of the given name
*/
void WriteNewNames (string tokens[], int tokenCount)
{
    // Check for at least one argument
    if (tokenCount < 2)
    {
        cout << "HALshell: " << tokens[0] << " requires at least one argument" << endl;
        return;
    }

    string filename = AccumulateTokens(tokens, tokenCount);

    // Write the aliases to file
    ofstream file;
    file.open(filename.c_str());
    if (!file)
    {
        cout << "HALshell: failed to open " << filename << endl;
        return;
    }

    // Iterate over the new name list
    for (vector< vector<string> >::iterator oit = shellInfo.newNames.begin();
         oit != shellInfo.newNames.end();
         ++oit)
    {
        // Iterate over each entries' tokens
        for (vector<string>::iterator iit = oit->begin();
             iit != oit->end();
             ++iit)
        {
            file << *iit << " ";
        }
        file << endl;
    }
    file.close();

    return;
}

/*
    Function: ReadNewNames
    Input:
        tokens - the tokens that have been passed from the command line
        tokenCount - the count of the tokens passed
    Output:
        global shellInfo updated
    Purpose:
        Read the list of new aliases contained within the file
        into the shell's alias list, deleting the shell's previous alias list.
*/
void ReadNewNames (string tokens[], int tokenCount)
{
    // Check for at least one argument
    if (tokenCount < 2)
    {
        cout << "HALshell: " << tokens[0] << " requires at least one argument" << endl;
        return;
    }

    string filename = AccumulateTokens(tokens, tokenCount);
    
    // Read from the specified file
    ifstream file;
    file.open(filename.c_str());
    if (!file)
    {
        cout << "HALshell: failed to open " << filename << endl;
        return;
    }

    vector< vector<string> > aliases;
    string line;

    // Get the contents of each line as a separate alias
    // Stop when the new name limit is reached
    while (aliases.size() < shellInfo.newNameSize && getline(file, line))
    {
        string fileTokens[MAX_COMMAND_LINE_ARGUMENTS];
        int fileTokenCount = TokenizeCommandLine(fileTokens, line);
        vector<string> lineVector;

        // Move the line into a vector
        for (int i = 0; i < fileTokenCount; ++i)
        {
            lineVector.push_back(fileTokens[i]);
        }

        // Add the vector to the list of new names
        aliases.push_back(lineVector);
    }

    // If there are more lines then they exceed the limit
    if (getline(file, line))
    {
        cout << "HALshell: new name limit reached, some aliases were not added" << endl;
    }
    file.close();

    shellInfo.newNames = aliases;

    return;
}

/*
    Function: RestoreDefaults
    Input:
        tokens - the tokens that have been passed from the command line
        tokenCount - the count of the tokens passed
    Output:
        global shellInfo updated
    Purpose:
        Restore all of the shell's settings to their default values
*/
void RestoreDefaults (string tokens[], int tokenCount)
{
    // Check there are no arguments
    if (tokenCount > 1)
    {
        cout << "HALshell: " << tokens[0] << " only requires one argument" << endl;
        return;
    }

    // Set shell options to hardcoded defaults

    string defaultTokens[] = {"RestoreDefaults_ERROR", ""};
    int defaultTokenCount = 2;

    defaultTokens[1] = "HALshell";
    SetShellName(defaultTokens, defaultTokenCount);

    defaultTokens[1] = ">";
    SetTerminator(defaultTokens, defaultTokenCount);

    defaultTokens[1] = "10";
    SetHistorySize(defaultTokens, defaultTokenCount);
    SetNewNameSize(defaultTokens, defaultTokenCount);

    return;
}

#pragma endregion Custom_Commands




/*********************************
 *                               *
 *    Miscellaneous Functions    *
 *        and Procedures         *
 *                               *
 *********************************/


#pragma region Miscellaneous_Functions_and_Procedures


/*
    Function: AccumulateTokens
    Input:
        tokens - the tokens that have been passed from the command line
        tokenCount - the count of the tokens passed
        start (optional) - the index to start from
        end (optional) - the index to end on
    Output:
        string - the accumulated tokens
    Purpose:
        Accumulate the tokens from the given token array into a string.
*/
string AccumulateTokens(string tokens[], int tokenCount,
                        int start, int end)
{
    string accumulatedTokens = "";

    // Set default end to the number of tokens
    if (end == -1)
    {
        end = tokenCount;
    }

    // Check that proper bounds are provided
    if (start >= end)
    {
        cout << "ERROR: illegal bounds in AccumulateTokens" << endl
            << "start: " << start << " end: " << end << endl
            << "exiting..." << endl;
        exit(1);
    }

    // Accumulate the tokens
    for (int i = start; i < end; ++i)
    {
        accumulatedTokens += tokens[i];
        accumulatedTokens += " ";
    }

    // Remove the erroneous last space
    accumulatedTokens.erase(accumulatedTokens.end() - 1);

    return accumulatedTokens;
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

string itos (int i)
{
    stringstream s;

    s << i;

    return s.str ();
}

#pragma endregion Miscellaneous_Functions_and_Procedures
