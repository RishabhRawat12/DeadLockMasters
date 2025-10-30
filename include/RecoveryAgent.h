#pragma once

#include <map>

using namespace std;

// Forward declarations.
class ResourceManager;
class Process;

// Handles deadlock recovery actions.
class RecoveryAgent
{
private:
    Process *lastVictimProcess = nullptr;
    map<int, int> lastVictimPreemptedResources;

public:
    // Attempt recovery. Returns true on success.
    bool initiateRecovery(ResourceManager &rm);

    // Get results of last recovery.
    Process *getVictimProcess();
    map<int, int> getPreemptedResources();
};