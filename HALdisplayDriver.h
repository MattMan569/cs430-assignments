//
// HALdisplayDriver.h
//
// Copyright (c) 2019 Robert J. Hilderman.
// All Rights Reserved.
//

#define HAL_DISPLAY_DRIVER_H

#include <iostream>
#include <fstream>
#include <stdlib.h>
#include <signal.h>
#include <sys/types.h>
#include <unistd.h>

using namespace std;

#ifndef HAL_SIGNALS_H
    #include "HALsignals.h"
#endif

// Global Variables

pid_t HALosPid;
string pid = "";
string systemCall = "";
string buffer = "";
string result = "";

// The "Display Driver"

void HALdisplayDriver ();
void ProcessIORequest ();

// The "Communication Media"

void GetMessageFromHALos ();
void SendMessageToHALos ();

/*

  #####
 #     #
#       # THE CODE THAT FOLLOWS SHOULD NOT BE CHANGED IN ANY WAY!
# STOP! #
#       # IF YOUR O/S IS NOT WORKING, YOU WON'T FIND THE PROBLEM HERE!
 #     #
  #####

*/

void Initialize ();

volatile sig_atomic_t messageFromHALos = 0;

int main ()
{
    Initialize ();
    HALdisplayDriver ();

    return 0;
}

void Initialize ()
{
    if ((sigemptyset (&interruptMask) == -1) ||
        (sigaddset (&interruptMask, SIGRTMIN) == -1))
    {
        cout << "HALdisplayDriver: unable to initialize signal mask" << endl;
        exit (1);
    }
    act.sa_sigaction = &SignalHandler;
    act.sa_mask = interruptMask;
    act.sa_flags = SA_SIGINFO;
    if ((sigemptyset (&act.sa_mask) == -1) ||
        (sigaction (SIGRTMIN, &act, NULL) == -1))
    {
        cout << "HALdisplayDriver: unable to connect to HALos" << endl;
        exit (1);
    }
    signal (SIGINT, SIG_IGN);

    HALosPid = getppid ();

    return;
}

static void SignalHandler (int signalNo, siginfo_t *info, void *context)
{
    if (signalNo == SIGRTMIN)
    {
        if (info -> si_pid == HALosPid)
        {
            messageFromHALos = 1;
        }
    }
}
