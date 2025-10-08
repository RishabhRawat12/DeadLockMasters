#pragma once
#include <vector>
#include <map>
#include "Process.h"
#include "Resource.h"
#include "DeadlockDetector.h"
#include "RecoveryAgent.h"
#include "StarvationGuardian.h"

// Forward declarations are a C++ trick to tell the compiler that these classes exist,
// without needing to include their full header files. This helps prevent "circular dependency" errors.
class DeadlockDetector;
class RecoveryAgent;
class StarvationGuardian;

// This is the main orchestrator of our entire simulation.
// It acts like the OS kernel, managing all processes, resources, and handling all requests.
class ResourceManager {
public:
    // The master lists of all processes and resources in our system.
    std::vector<Process> processes;
    std::vector<Resource> resources;

    // A crucial data structure to track which processes are waiting for which resources.
    // Key: resourceId, Value: a list of processIds waiting for it.
    std::map<int, std::vector<int>> waitingProcesses;

    // Our specialized modules for handling specific OS tasks.
    DeadlockDetector detector;
    RecoveryAgent recoveryAgent;
    StarvationGuardian starvationGuardian;

    // Constructor to initialize the resource manager.
    ResourceManager();

    // Methods to set up the initial state of the system.
    void addProcess(const Process& p);
    void addResource(const Resource& r);

    // The core functions that drive the simulation.
    bool requestResource(int processId, int resourceId, int count);
    bool releaseResource(int processId, int resourceId, int count);

    // Helper functions to easily find a process or resource by its ID.
    Process* findProcessById(int processId);
    Resource* findResourceById(int resourceId);

    // Prints a neat summary of the current system state to the console.
    void printState();
};