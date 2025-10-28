#include "../include/ResourceManager.h"
#include <iostream>
#include <algorithm>
#include <vector>
#include <map>
#include <limits> // Required for numeric_limits
#include <utility> // Required for std::pair

using namespace std;

// Constructor
ResourceManager::ResourceManager() : starvationGuardian(5) {}

void ResourceManager::addProcess(const Process& p) {
    processes.push_back(p);
}

void ResourceManager::addResource(const Resource& r) {
    resources.push_back(r);
}

// Helper to find process by ID.
Process* ResourceManager::findProcessById(int processId) {
    for (auto& p : processes) {
        if (p.id == processId) {
            return &p;
        }
    }
    return nullptr;
}

// Helper to find resource by ID.
Resource* ResourceManager::findResourceById(int resourceId) {
    for (auto& r : resources) {
        if (r.id == resourceId) {
            return &r;
        }
    }
    return nullptr;
}

// Handles a process requesting resources.
bool ResourceManager::requestResource(int processId, int resourceId, int count, int retryCount /*=0*/) {
    cout << "Process " << processId << " requests " << count << " of Resource " << resourceId << endl;

    if (count <= 0) {
        cerr << "Error: Process " << processId << " requested invalid count (" << count << ")." << endl;
        return false;
    }

    Process* process = findProcessById(processId);
    Resource* resource = findResourceById(resourceId);

    if (!process || !resource) {
        cerr << "Error: Invalid process (" << processId << ") or resource (" << resourceId << ") ID." << endl;
        return false;
    }

    if (resource->availableInstances >= count) {
        resource->availableInstances -= count;
        process->resourcesHeld[resourceId] += count;
        process->resetWaitTime();
        cout << "Request GRANTED." << endl;
        return true;
    } else {
        cout << "Request DENIED. Not enough resources. Process " << processId << " must wait." << endl;
        auto& queue = waitingProcesses[resourceId];
        bool alreadyWaiting = false;
        for(const auto& waitEntry : queue) {
            if (waitEntry.first == processId) {
                 alreadyWaiting = true;
                 break;
            }
        }
        if (!alreadyWaiting) {
             queue.push_back({processId, count});
        }

        if (detector.hasCycle(*this)) {
             if (retryCount >= MAX_REQUEST_RETRIES) {
                 cerr << "FATAL ERROR: Deadlock detected and recovery failed after " << MAX_REQUEST_RETRIES << " retries." << endl;
                  auto& q = waitingProcesses[resourceId];
                  q.erase(remove_if(q.begin(), q.end(), [processId](const std::pair<int, int>& p){ return p.first == processId; }), q.end());
                 return false;
             }
            recoveryAgent.initiateRecovery(*this);
            cout << "Retrying request for Process " << processId << " after deadlock recovery (Retry " << retryCount + 1 << ")" << endl;
            return requestResource(processId, resourceId, count, retryCount + 1);
        }
        return false;
    }
}

// Handles a process releasing resources.
bool ResourceManager::releaseResource(int processId, int resourceId, int count) {
    cout << "Process " << processId << " releases " << count << " of Resource " << resourceId << endl;

     if (count <= 0) {
        cerr << "Error: Process " << processId << " tried to release invalid count (" << count << ")." << endl;
        return false;
    }

    Process* process = findProcessById(processId);
    Resource* resource = findResourceById(resourceId);

    if (!process || !resource) {
        cerr << "Error: Invalid process or resource ID during release." << endl;
        return false;
    }

    if (process->resourcesHeld.count(resourceId) && process->resourcesHeld[resourceId] >= count) {
        process->resourcesHeld[resourceId] -= count;
        if (process->resourcesHeld[resourceId] == 0) {
            process->resourcesHeld.erase(resourceId);
        }
        resource->availableInstances += count;
        cout << "Resource released successfully. R" << resourceId << " available: " << resource->availableInstances << endl;
        notifyWaitingProcesses();
        return true;
    } else {
        cerr << "Error: Process " << processId << " cannot release " << count << " of R" << resourceId
             << " (holds " << (process->resourcesHeld.count(resourceId) ? process->resourcesHeld[resourceId] : 0) << ")." << endl;
        return false;
    }
}

// Centralized function to check and notify waiting processes based on priority.
void ResourceManager::notifyWaitingProcesses() {
    cout << "--- Checking waiting processes ---" << endl;
    bool resourceGranted;
    do {
        resourceGranted = false;
        int bestProcId = -1;
        int bestResId = -1;
        int bestReqCount = -1;
        int highestPriority = -1; // Initialize lower than any possible priority

        // Use standard C++11 range-based for loop for map iteration
        for (auto const& map_pair : waitingProcesses) {
            int resId = map_pair.first;
            const auto& queue = map_pair.second; // vector<pair<int, int>>

            Resource* resource = findResourceById(resId);
            if (!resource || resource->availableInstances == 0) continue;

            int currentHighestPriority = -1;
            int candidateProcId = -1;
            int candidateReqCount = -1;
            for (const auto& waitEntry : queue) {
                Process* process = findProcessById(waitEntry.first);
                if (process && resource->availableInstances >= waitEntry.second) {
                     if (process->priority > currentHighestPriority) {
                         currentHighestPriority = process->priority;
                         candidateProcId = waitEntry.first;
                         candidateReqCount = waitEntry.second;
                     }
                }
            }

             if (candidateProcId != -1 && currentHighestPriority > highestPriority) {
                 highestPriority = currentHighestPriority;
                 bestProcId = candidateProcId;
                 bestResId = resId;
                 bestReqCount = candidateReqCount;
             }
        }

        if (bestProcId != -1) {
            cout << "Notifying highest priority waiting Process " << bestProcId << " (Priority: " << highestPriority << ") for R" << bestResId << endl;

            auto& queue = waitingProcesses[bestResId];
            queue.erase(remove_if(queue.begin(), queue.end(),
                                  [bestProcId](const std::pair<int,int>& p){ return p.first == bestProcId; }),
                        queue.end());

            if (requestResource(bestProcId, bestResId, bestReqCount)) {
                resourceGranted = true;
            } else {
                 cerr << "Error: Failed grant after notification for P" << bestProcId << ". Re-adding to wait list." << endl;
                 waitingProcesses[bestResId].push_back({bestProcId, bestReqCount});
            }
        }

        // Clean up empty queues
        for (auto it = waitingProcesses.begin(); it != waitingProcesses.end(); ) {
            if (it->second.empty()) {
                it = waitingProcesses.erase(it);
            } else {
                ++it;
            }
        }

    } while (resourceGranted);
     cout << "--- Finished checking waiting processes ---" << endl;
}

// Prints the current system state.
void ResourceManager::printState() {
    cout << "\n==================== SYSTEM STATE ====================" << endl;
    cout << "--- Resources ---" << endl;
    for (const auto& r : resources) {
        cout << "Resource " << r.id << ": Total=" << r.totalInstances << ", Available=" << r.availableInstances << endl;
    }
    cout << "\n--- Processes ---" << endl;
    for (const auto& p : processes) {
        cout << "Process " << p.id << " (Priority: " << p.priority << "):" << endl;
        if (p.resourcesHeld.empty()) {
            cout << "  - Holding no resources." << endl;
        } else {
            for (const auto& pair : p.resourcesHeld) {
                cout << "  - Holding " << pair.second << " of Resource " << pair.first << endl;
            }
        }
    }
     cout << "\n--- Waiting Processes ---" << endl;
    bool anyWaiting = false;
    // Use standard C++11 range-based for loop for map iteration
    for (const auto& map_pair : waitingProcesses) {
        if (!map_pair.second.empty()) {
            anyWaiting = true;
            cout << "Resource " << map_pair.first << " is awaited by: ";
            for (const auto& waitEntry : map_pair.second) {
                cout << "P" << waitEntry.first << "(" << waitEntry.second << ") ";
            }
            cout << endl;
        }
    }
    if(!anyWaiting){
        cout << "No processes are currently waiting for resources." << endl;
    }
    cout << "======================================================\n" << endl;
}