#pragma once
#include <vector>
#include <map>

// Represents a single process in our operating system simulation.
// Each process has a unique ID and keeps track of the resources it's using.
class Process {
public:
    int id;
    
    // Priority is used for our starvation-prevention "Aging" technique.
    // A process that waits too long will have its priority increased.
    int priority;

    // Used to track when a process started waiting to detect potential starvation.
    long long waitStartTime;

    // A map to store the resources this process currently holds.
    // Key: resourceId, Value: number of instances held.
    std::map<int, int> resourcesHeld;

    // This is for the Banker's Algorithm, where each process must declare
    // the maximum number of resources it might ever need.
    std::map<int, int> maxResourcesNeeded;

    // Constructor to create a new process with a given ID.
    Process(int processId);

    // Bumps up the process's priority.
    void increasePriority();

    // Resets the wait timer, usually called when a process gets the resource it was waiting for.
    void resetWaitTime();
};