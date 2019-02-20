//
// HALsignals.h
//
// Copyright (c) 2019 Robert J. Hilderman.
// All Rights Reserved.
//

#define HAL_SIGNALS_H

static void SignalHandler (int signalNo, siginfo_t *info, void *context);
sigset_t interruptMask;
struct sigaction act;

inline void BlockSignals (string source)
{
    if (sigprocmask (SIG_BLOCK, &interruptMask, NULL) == -1)
    {
        cout << source << ": unable to block signals" << endl;
        exit (1);
    }
}

inline void UnblockSignals (string source)
{
    if (sigprocmask (SIG_UNBLOCK, &interruptMask, NULL) == -1)
    {
        cout << source << ": unable to unblock signals" << endl;
        exit (1);
    }
}

