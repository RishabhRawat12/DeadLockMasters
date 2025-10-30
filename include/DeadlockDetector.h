#pragma once

#include <vector>

using namespace std;

// Forward declaration.
class ResourceManager;

// Detects or avoids deadlocks.
class DeadlockDetector
{
public:
    // Banker's Algorithm (avoidance).
    bool isSafeState(ResourceManager &rm);

    // Wait-for graph (detection).
    bool hasCycle(ResourceManager &rm);
};