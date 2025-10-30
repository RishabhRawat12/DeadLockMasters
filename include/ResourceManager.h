#pragma once

#include <vector>
#include <map>
#include <list>
#include <string>
#include "Process.h"
#include "Resource.h"
#include "DeadlockDetector.h"
#include "RecoveryAgent.h"
#include "StarvationGuardian.h"

using namespace std;

// Info on a waiting process.
struct WaitingInfo
{
    int processId;
    int count;
    WaitingInfo(int pId, int c) : processId(pId), count(c) {}
};

// Enum for strategy selection.
enum class DeadlockStrategy
{
    DETECT, // Use Detection & Recovery
    AVOID   // Use Avoidance (Banker's)
};

// Main class to manage the simulation.
class ResourceManager
{
public:
    vector<Process> processes;
    vector<Resource> resources;

    // <ResourceID, List of Waiting Processes>
    map<int, list<WaitingInfo>> waitingProcesses;

    // Component modules.
    DeadlockDetector detector;
    RecoveryAgent recoveryAgent;
    StarvationGuardian starvationGuardian;

    // Current strategy.
    DeadlockStrategy strategy = DeadlockStrategy::DETECT;

    // Log for the GUI.
    list<string> logMessages;

    ResourceManager();

    // Set the deadlock strategy.
    void setStrategy(DeadlockStrategy newStrategy);

    // Add components.
    void addProcess(const Process &p);
    void addResource(const Resource &r);

    // Set max resource needs (Banker's).
    void declareMaxResources(int processId, int resourceId, int maxCount);

    // Core simulation events.
    bool requestResource(int processId, int resourceId, int count);
    bool releaseResource(int processId, int resourceId, int count);

    // Find components.
    Process *findProcessById(int processId);
    Resource *findResourceById(int resourceId);

    // Check wait list after a release.
    void checkWaitingProcesses(int resourceId);

    // Trigger aging check.
    void applyAgingToWaitingProcesses();

    // Add a message to the GUI log.
    void log(string message);

    // Getters for Banker's Algorithm.
    const vector<Process> &getProcesses() const { return processes; }
    const vector<Resource> &getResources() const { return resources; }
    const map<int, list<WaitingInfo>> &getWaitingProcesses() const { return waitingProcesses; }
};