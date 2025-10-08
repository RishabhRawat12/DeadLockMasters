#pragma once

// Forward declaration, as ResourceManager will be passed to this class's methods.
class ResourceManager;

// This class is dedicated to one thing: detecting deadlocks.
// It contains the logic for different detection and avoidance algorithms.
class DeadlockDetector {
public:
    // Implements the Banker's Algorithm to check if the system is in a "safe state".
    // This is a deadlock *avoidance* technique.
    bool isSafeState(ResourceManager& rm);

    // Implements a graph-based algorithm (using DFS) to find a circular wait.
    // This is a deadlock *detection* technique.
    bool hasCycle(ResourceManager& rm);
};