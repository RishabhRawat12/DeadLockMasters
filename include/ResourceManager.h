#ifndef RESOURCEMANAGER_H
#define RESOURCEMANAGER_H

#include <vector>
#include <map>
#include <string> // Added for storing request details
#include <tuple>  // Added for storing request details

#include "Process.h"
#include "Resource.h"
#include "DeadlockDetector.h"
#include "RecoveryAgent.h"
#include "StarvationGuardian.h"

// Forward declarations
class DeadlockDetector;
class RecoveryAgent;
class StarvationGuardian;

// Main orchestrator managing processes, resources, requests, and deadlock/starvation handling.
class ResourceManager {
public:
    std::vector<Process> processes;
    std::vector<Resource> resources;

    // Key: resourceId, Value: list of waiting {processId, requested_count} pairs.
    std::map<int, std::vector<std::pair<int, int>>> waitingProcesses;

    DeadlockDetector detector;
    RecoveryAgent recoveryAgent;
    StarvationGuardian starvationGuardian; // Needs initialization

    ResourceManager(); // Consider initializing StarvationGuardian here

    void addProcess(const Process& p);
    void addResource(const Resource& r);

    // Handles resource requests, returns true if granted immediately.
    // Includes deadlock detection and recovery trigger.
    bool requestResource(int processId, int resourceId, int count, int retryCount = 0); // Added retry counter

    // Handles resource releases, returns true on success.
    bool releaseResource(int processId, int resourceId, int count);

    // Helper to find a process by ID (returns pointer for modification).
    Process* findProcessById(int processId);
    // Helper to find a resource by ID (returns pointer for modification).
    Resource* findResourceById(int resourceId);

    // Prints the current state of resources and processes.
    void printState();

    // Centralized function to notify waiting processes after resources are freed.
    void notifyWaitingProcesses(); // Added helper function

private:
    const int MAX_REQUEST_RETRIES = 3; // Max retries after deadlock recovery
};

#endif // RESOURCEMANAGER_H