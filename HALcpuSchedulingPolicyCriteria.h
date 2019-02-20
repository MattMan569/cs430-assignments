//
// HALcpuSchedulingPolicyCriteria.h
//
// Copyright (c) 2019 Robert J. Hilderman.
// All Rights Reserved.
//

#define HAL_CPU_SCHEDULING_POLICY_CRITERIA_H

struct cpuSchedulingPolicyCriteria
{
    string cpuSchedulingPolicy;
    int quantumLengthMultiplier;
    int interruptsUntilMoveDown;
    int interruptsUntilMoveUp;
};
