#ifndef PROCESS_H
#define PROCESS_H

#include <vector> // Keep standard includes if needed by map/vector indirectly
#include <map>

// Represents a single process in the simulation.
class Process {
public:
    int id;
    int priority;           // Used for Aging technique. Higher value = higher priority.
    long long waitStartTime; // Timestamp when process started waiting. 0 if not waiting.

    // Key: resourceId, Value: number of instances held.
    std::map<int, int> resourcesHeld;

    // Key: resourceId, Value: maximum instances needed (for Banker's Algorithm).
    std::map<int, int> maxResourcesNeeded;

    Process(int processId);

    // Increases the process's priority.
    void increasePriority();

    // Resets the wait timer.
    void resetWaitTime();
};

#endif // PROCESS_H