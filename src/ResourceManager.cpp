#include "../include/ResourceManager.h"
#include <iostream>
#include <algorithm>
#include <vector>
#include <map>
#include <string> // for to_string

using namespace std;

// Constructor.
ResourceManager::ResourceManager() {}

// Add a log message for the GUI.
void ResourceManager::log(string message)
{
    // We add messages to the front so the GUI log shows newest first.
    // Or, append and have GUI auto-scroll. Let's append.
    logMessages.push_back(message);
}

// Set the active deadlock strategy.
void ResourceManager::setStrategy(DeadlockStrategy newStrategy)
{
    this->strategy = newStrategy;
    if (this->strategy == DeadlockStrategy::AVOID)
    {
        log("[Strategy: Deadlock AVOIDANCE (Banker's Algorithm)]");
    }
    else
    {
        log("[Strategy: Deadlock DETECTION & RECOVERY (Graph Cycle)]");
    }
}

// Add a process.
void ResourceManager::addProcess(const Process &p)
{
    processes.push_back(p);
}

// Add a resource.
void ResourceManager::addResource(const Resource &r)
{
    resources.push_back(r);
}

// Declare max needs (Banker's).
void ResourceManager::declareMaxResources(int processId, int resourceId, int maxCount)
{
    Process *process = findProcessById(processId);
    Resource *resource = findResourceById(resourceId);
    if (process && resource)
    {
        if (maxCount > resource->totalInstances)
        {
            log("Warning: P" + to_string(processId) + " max (" + to_string(maxCount) + ") for R" + to_string(resourceId) + " > total (" + to_string(resource->totalInstances) + "). Clamping.");
            maxCount = resource->totalInstances;
        }
        process->maxResourcesNeeded[resourceId] = maxCount;
        log("  - P" + to_string(processId) + " declared max R" + to_string(resourceId) + ": " + to_string(maxCount));
    }
    else
    {
        log("Warning: Invalid P/R ID for max declaration.");
    }
}

// Find process.
Process *ResourceManager::findProcessById(int processId)
{
    for (auto &p : processes)
    {
        if (p.id == processId)
            return &p;
    }
    return nullptr;
}

// Find resource.
Resource *ResourceManager::findResourceById(int resourceId)
{
    for (auto &r : resources)
    {
        if (r.id == resourceId)
            return &r;
    }
    return nullptr;
}

// Handle resource request.
bool ResourceManager::requestResource(int processId, int resourceId, int count)
{
    log("P" + to_string(processId) + " requests " + to_string(count) + " of R" + to_string(resourceId));
    Process *process = findProcessById(processId);
    Resource *resource = findResourceById(resourceId);

    if (!process || !resource)
    {
        log("Error: Invalid P/R ID in request.");
        return false;
    }
    if (count <= 0)
    {
        log("Error: Request count must be > 0.");
        return false;
    }

    // --- STRATEGY SWITCH ---
    if (strategy == DeadlockStrategy::AVOID)
    {
        // --- Banker's Algorithm (Avoidance) Logic ---
        int max_need = 0;
        if (process->maxResourcesNeeded.count(resourceId))
        {
            max_need = process->maxResourcesNeeded.at(resourceId);
        }
        else
        {
            log("Error: P" + to_string(processId) + " requested R" + to_string(resourceId) + " but has no max need declared.");
            return false;
        }

        int current_allocation = 0;
        if (process->resourcesHeld.count(resourceId))
        {
            current_allocation = process->resourcesHeld.at(resourceId);
        }
        if (count + current_allocation > max_need)
        {
            log("Error: P" + to_string(processId) + " request exceeds declared max need.");
            return false;
        }

        if (count <= resource->availableInstances)
        {
            log("  - Tentatively allocating for safety check...");
            resource->availableInstances -= count;
            process->resourcesHeld[resourceId] += count;

            if (detector.isSafeState(*this))
            {
                log("Request GRANTED (Safe state).");
                process->resetWaitTime();
                return true;
            }
            else
            {
                log("  - Rolling back (Unsafe state).");
                resource->availableInstances += count;
                process->resourcesHeld[resourceId] -= count;
                if (process->resourcesHeld.at(resourceId) == 0)
                {
                    process->resourcesHeld.erase(resourceId);
                }
                log("Request DENIED (Unsafe). P" + to_string(processId) + " must wait.");
                bool alreadyWaiting = false;
                if (waitingProcesses.count(resourceId))
                {
                    for (const auto &info : waitingProcesses.at(resourceId))
                    {
                        if (info.processId == processId)
                        {
                            alreadyWaiting = true;
                            break;
                        }
                    }
                }
                if (!alreadyWaiting)
                {
                    waitingProcesses[resourceId].emplace_back(processId, count);
                }
                applyAgingToWaitingProcesses();
                return false;
            }
        }
        else
        {
            log("Request DENIED (Not enough). P" + to_string(processId) + " must wait.");
            bool alreadyWaiting = false;
            if (waitingProcesses.count(resourceId))
            {
                for (const auto &info : waitingProcesses.at(resourceId))
                {
                    if (info.processId == processId)
                    {
                        alreadyWaiting = true;
                        break;
                    }
                }
            }
            if (!alreadyWaiting)
            {
                waitingProcesses[resourceId].emplace_back(processId, count);
            }
            applyAgingToWaitingProcesses();
            return false;
        }
    }
    else
    {
        // --- Default: Deadlock Detection & Recovery Logic ---
        if (resource->availableInstances >= count)
        {
            resource->availableInstances -= count;
            process->resourcesHeld[resourceId] += count;
            log("Request GRANTED.");
            process->resetWaitTime();
            return true;
        }
        else
        {
            log("Request DENIED (Not enough). P" + to_string(processId) + " waits.");
            bool alreadyWaiting = false;
            if (waitingProcesses.count(resourceId))
            {
                for (const auto &info : waitingProcesses.at(resourceId))
                {
                    if (info.processId == processId)
                    {
                        alreadyWaiting = true;
                        break;
                    }
                }
            }
            if (!alreadyWaiting)
            {
                waitingProcesses[resourceId].emplace_back(processId, count);
            }
            applyAgingToWaitingProcesses();

            if (detector.hasCycle(*this))
            {
                bool recovery_ok = recoveryAgent.initiateRecovery(*this);
                if (recovery_ok)
                {
                    log("  - Post-recovery: Checking wait queues.");
                    map<int, int> preempted = recoveryAgent.getPreemptedResources();
                    for (const auto &pair : preempted)
                    {
                        checkWaitingProcesses(pair.first);
                    }
                }
                else
                {
                    log("*** CRITICAL: Deadlock detected but RECOVERY FAILED! ***");
                }
            }
            return false;
        }
    }
}

// Handle resource release.
bool ResourceManager::releaseResource(int processId, int resourceId, int count)
{
    log("P" + to_string(processId) + " releases " + to_string(count) + " of R" + to_string(resourceId));
    Process *process = findProcessById(processId);
    Resource *resource = findResourceById(resourceId);

    if (!process || !resource)
    {
        log("Error: Invalid P/R ID in release.");
        return false;
    }
    if (count <= 0)
    {
        log("Error: Release count must be > 0.");
        return false;
    }

    if (process->resourcesHeld.count(resourceId) && process->resourcesHeld.at(resourceId) >= count)
    {
        process->resourcesHeld[resourceId] -= count;
        if (process->resourcesHeld[resourceId] == 0)
            process->resourcesHeld.erase(resourceId);
        resource->availableInstances += count;
        log("R" + to_string(resourceId) + " released (Available: " + to_string(resource->availableInstances) + ").");

        checkWaitingProcesses(resourceId);
        applyAgingToWaitingProcesses();
        return true;
    }
    else
    {
        log("Error: P" + to_string(processId) + " cannot release " + to_string(count) + " of R" + to_string(resourceId) + " (Holds: " + (process->resourcesHeld.count(resourceId) ? to_string(process->resourcesHeld.at(resourceId)) : "0") + ").");
        return false;
    }
}

// Check waiting list.
void ResourceManager::checkWaitingProcesses(int resourceId)
{
    Resource *resource = findResourceById(resourceId);
    if (!resource || waitingProcesses.find(resourceId) == waitingProcesses.end() || waitingProcesses.at(resourceId).empty())
    {
        return;
    }

    auto &waiting_list = waitingProcesses.at(resourceId);
    log("  - Checking waits for R" + to_string(resourceId) + " (Available: " + to_string(resource->availableInstances) + ")");

    for (auto it = waiting_list.begin(); it != waiting_list.end(); /* manual */)
    {
        WaitingInfo &info = *it;
        Process *waitingProcess = findProcessById(info.processId);

        if (!waitingProcess)
        {
            it = waiting_list.erase(it);
            continue;
        }

        if (resource->availableInstances >= info.count)
        {
            if (strategy == DeadlockStrategy::AVOID)
            {
                // --- Banker's: Check safety before granting to waiter ---
                log("  - Tentatively granting to P" + to_string(info.processId) + " (pending safety check)...");
                resource->availableInstances -= info.count;
                waitingProcess->resourcesHeld[resourceId] += info.count;

                if (detector.isSafeState(*this))
                {
                    log("    - Granting " + to_string(info.count) + " of R" + to_string(resourceId) + " to P" + to_string(info.processId) + " (Safe).");
                    waitingProcess->resetWaitTime();
                    it = waiting_list.erase(it);
                }
                else
                {
                    log("    - Cannot grant to P" + to_string(info.processId) + " (unsafe). Rolling back.");
                    resource->availableInstances += info.count;
                    waitingProcess->resourcesHeld[resourceId] -= info.count;
                    if (waitingProcess->resourcesHeld.at(resourceId) == 0)
                        waitingProcess->resourcesHeld.erase(resourceId);
                    ++it;
                }
            }
            else
            {
                // --- Detection: Grant if available ---
                log("    - Granting " + to_string(info.count) + " of R" + to_string(resourceId) + " to P" + to_string(info.processId) + ".");
                resource->availableInstances -= info.count;
                waitingProcess->resourcesHeld[resourceId] += info.count;
                waitingProcess->resetWaitTime();
                it = waiting_list.erase(it);
            }
        }
        else
        {
            ++it; // Not enough, check next waiter.
        }
    }
    if (waiting_list.empty())
        waitingProcesses.erase(resourceId);
}

// Trigger aging check.
void ResourceManager::applyAgingToWaitingProcesses()
{
    bool anyWaiting = false;
    for (const auto &pair : waitingProcesses)
    {
        if (!pair.second.empty())
        {
            anyWaiting = true;
            break;
        }
    }
    if (anyWaiting)
    {
        log("--- Applying Aging Check ---");
        starvationGuardian.applyAging(*this);
        // Note: StarvationGuardian adds its own logs.
    }
}