//
// HALfileTable.h
//
// Copyright (c) 2019 Robert J. Hilderman.
// All Rights Reserved.
//

#define HAL_FILE_TABLE_H

#include <string>

using namespace std;

#ifndef HAL_GLOBALS_H
    #include "HALglobals.h"
#endif

struct fileDescriptor
{
    string name;
    string address;
    string mode;
    string markerPosition;
};

class FileTableType
{
public:
    FileTableType ();
    ~FileTableType ();
    void ResetP ();
    void Iterate ();
    int GetP ();
    void SetP (int q);
    fileDescriptor Read ();
    void Write (string markerPosition);
    int Length ();
    bool IsEmpty ();
    bool IsFull ();
    void Insert (string name, string address, string mode);
    bool Find (string name, string address);
    void Delete ();
    void Clear ();
private:
    fileDescriptor fileTable [FILE_TABLE_SIZE];
    int p;
    int length;
};
