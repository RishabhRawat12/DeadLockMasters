#pragma once

#include <vector>
#include <map>
#include <string>

using namespace std;

// Represents a process in the simulation.
class Process
{
public:
    int id;
    int priority;
    long long waitStartTime;

    // <ResourceID, Count>
    map<int, int> resourcesHeld;

    // <ResourceID, MaxCount>
    map<int, int> maxResourcesNeeded;

    Process(int processId);
    void increasePriority();
    void resetWaitTime();
};