#pragma once

#include <fcntl.h>
#include <fstream>
#include <string>
#include <vector>

// THIS FILE CONTAINS THE CODE FOR A LOCK MECHANISM
// THAT ENSURES EXCLUSIVE ACCESS TO A FILE.

// THE FOLLOWING PROCEDURE IS FOR READING THE FILE.

namespace HALosHALshellFileIO
{
    // Shared file between HALos and HALshell
    const char* fileName = "HALosForHALshell";

    vector<string> ReadFile ()
    {
        int lock;
        struct flock key;

        // YOUR FILE VARIABLE DECLARATIONS GO HERE!
        ifstream inFile;

        key.l_type = F_WRLCK;
        key.l_whence = SEEK_SET;
        key.l_start = 0;
        key.l_len = 0;
        key.l_pid = getpid ();
        lock = open ("HALreturnStatusLock", O_WRONLY);
        fcntl (lock, F_SETLKW, &key);

        // YOUR FILE READ GOES HERE!
        inFile.open(fileName);
        if (!inFile)
        {
            cout << "HALosHALshell: cannot open background return buffer for reading" << endl;
            exit (1);
        }

        // Get the contents of the file
        // Every result is three lines
        vector<string> fileContent;
        string line;
        while (getline (inFile, line))
        {
            fileContent.push_back(line);
        }

        inFile.close();

        key.l_type = F_UNLCK;
        fcntl (lock, F_SETLK, &key);
        close (lock);

        // WHATEVER YOU NEED FROM THE FILE
        return (fileContent);
    }

    // THE FOLLOWING PROCEDURE IS FOR WRITING THE FILE.

    void WriteFile (vector<string> returnStatus)
    {
        int lock;
        struct flock key;

        // YOUR FILE VARIABLE DECLARATIONS GO HERE!
        ofstream outFile;

        key.l_type = F_WRLCK;
        key.l_whence = SEEK_SET;
        key.l_start = 0;
        key.l_len = 0;
        key.l_pid = getpid ();
        lock = open ("HALreturnStatusLock", O_WRONLY);
        fcntl (lock, F_SETLKW, &key);

        // YOUR FILE OPEN AND WRITE FOR HALosForHALshell GOES HERE!
        outFile.open(fileName, ofstream::out | ofstream::app);
        if (!outFile)
        {
            cout << "HALosHALshell: cannot open background return buffer for writing" << endl;
            exit (1);
        }

        // Move the result to the file
        for (vector<string>::iterator it = returnStatus.begin();
             it != returnStatus.end();
             ++it)
        {
            outFile << *it << endl;
        }

        outFile.close();

        key.l_type = F_UNLCK;
        fcntl (lock, F_SETLK, &key);
        close (lock);

        return;
    }
}
