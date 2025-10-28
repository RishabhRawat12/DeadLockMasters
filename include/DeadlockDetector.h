#ifndef DEADLOCKDETECTOR_H
#define DEADLOCKDETECTOR_H

#include <vector> // Include vector as it's used indirectly via ResourceManager

// Forward declaration
class ResourceManager;

// Class dedicated to detecting and potentially avoiding deadlocks.
class DeadlockDetector {
public:
    // Checks if the system is in a "safe state" using Banker's Algorithm (Placeholder).
    bool isSafeState(ResourceManager& rm);

    // Checks for a circular wait using DFS on the wait-for graph.
    bool hasCycle(ResourceManager& rm);
};

#endif // DEADLOCKDETECTOR_H