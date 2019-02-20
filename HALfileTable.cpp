//
// HALfileTable.cpp
//
// Copyright (c) 2019 Robert J. Hilderman.
// All Rights Reserved.
//

#include "HALfileTable.h"

FileTableType::FileTableType ()
{
    for (p = 0; p < FILE_TABLE_SIZE; p ++)
    {
        fileTable [p].name = "";
        fileTable [p].address = "";
        fileTable [p].mode = "";
        fileTable [p].markerPosition = "";
    }

    length = 0;
    p = -1;
}

FileTableType::~FileTableType ()
{
}

void FileTableType::ResetP ()
{
    p = -1;

    return;
}

void FileTableType::Iterate ()
{
    p ++;

    return;
}

int FileTableType::GetP ()
{
    return p;
}

void FileTableType::SetP (int q)
{
    p = q;

    return;
}

fileDescriptor FileTableType::Read ()
{
    return (fileTable [p]);
}

void FileTableType::Write (string markerPosition)
{
    fileTable [p].markerPosition = markerPosition;

    return;
}

int FileTableType::Length ()
{
    return length;
}

bool FileTableType::IsEmpty ()
{
    if (length == 0)
    {
        return true;
    }

    return false;
}

bool FileTableType::IsFull ()
{
    if (length == FILE_TABLE_SIZE)
    {
        return true;
    }

    return false;
}

void FileTableType::Insert (string name, string address, string mode)
{
    int q;

    p = 0;
    while (p < length && (name + address) > (fileTable [p].name + fileTable [p].address))
    {
        p ++;
    }

    q = length;
    while (q > p)
    {
        fileTable [q] = fileTable [q - 1];
        q --;
    }
    
    fileTable [p].name = name;
    fileTable [p].address = address;
    fileTable [p].mode = mode;
    if (mode == "input")
    {
        fileTable [p].markerPosition = "0";
    }
    else
    {
        fileTable [p].markerPosition = "";
    }

    length ++;

    return;
}

bool FileTableType::Find (string name, string address)
{
    int q;
    int r;

    q = 0;
    r = length - 1;
    while (q <= r)
    {
        p = (q + r) / 2;
        if ((name + address) < (fileTable [p].name + fileTable [p].address))
        {
            r = p - 1;
        }
        else if ((name + address) > (fileTable [p].name + fileTable [p].address))
        {
            q = p + 1;
        }
        else
        {
            return true;
        }
    }

    return false;
}

void FileTableType::Delete ()
{
    int q;

    q = p;
    p ++;
    while (p < length)
    {
        fileTable [p - 1] = fileTable [p];
        p ++;
    }

    length --;
    p = q;

    return;
}

void FileTableType::Clear ()
{
    for (p = 0; p < FILE_TABLE_SIZE; p ++)
    {
        fileTable [p].name = "";
        fileTable [p].address = "";
        fileTable [p].mode = "";
        fileTable [p].markerPosition = "";
    }

    length = 0;
    p = -1;
}

