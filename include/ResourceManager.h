#pragma once
#include <vector>
#include <map>
#include <utility> // For std::pair
#include "Process.h"
#include "Resource.h"
#include "DeadlockDetector.h"
#include "RecoveryAgent.h"
#include "StarvationGuardian.h"

// Forward declarations
class DeadlockDetector;
class RecoveryAgent;
class StarvationGuardian;

// Manages processes, resources, and handles requests/releases.
class ResourceManager {
public:
    std::vector<Process> processes;
    std::vector<Resource> resources;
    // Map: resourceId -> vector of {processId, requestedCount} pairs.
    std::map<int, std::vector<std::pair<int, int>>> waitingProcesses;

    DeadlockDetector detector;
    RecoveryAgent recoveryAgent;
    StarvationGuardian starvationGuardian;

    ResourceManager();

    void addProcess(const Process& p);
    void addResource(const Resource& r);
    bool requestResource(int processId, int resourceId, int count);
    bool releaseResource(int processId, int resourceId, int count);
    Process* findProcessById(int processId);
    Resource* findResourceById(int resourceId);
    void printState(); // Prints current system state.
};