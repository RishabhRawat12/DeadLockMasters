#include "../include/ResourceManager.h"
#include <iostream>
#include <algorithm>

using namespace std;

ResourceManager::ResourceManager() {}

void ResourceManager::addProcess(const Process& p) {
    processes.push_back(p);
}

void ResourceManager::addResource(const Resource& r) {
    resources.push_back(r);
}

// A helper function to find a process object by its ID.
// We return a pointer so we can directly modify the process.
Process* ResourceManager::findProcessById(int processId) {
    for (auto& p : processes) {
        if (p.id == processId) {
            return &p;
        }
    }
    return nullptr;
}

// A helper function to find a resource object by its ID.
Resource* ResourceManager::findResourceById(int resourceId) {
    for (auto& r : resources) {
        if (r.id == resourceId) {
            return &r;
        }
    }
    return nullptr;
}

// This is one of the most important functions. It handles a process's request for a resource.
bool ResourceManager::requestResource(int processId, int resourceId, int count) {
    cout << "Process " << processId << " requests " << count << " of Resource " << resourceId << endl;
    Process* process = findProcessById(processId);
    Resource* resource = findResourceById(resourceId);

    if (!process || !resource) {
        cerr << "Error: Invalid process or resource ID." << endl;
        return false;
    }

    // The main question: are there enough resources available right now?
    if (resource->availableInstances >= count) {
        // If yes, great! We can grant the request.
        resource->availableInstances -= count;
        process->resourcesHeld[resourceId] += count;
        cout << "Request GRANTED." << endl;
        return true;
    } else {
        // If no, the process must wait. This is where a deadlock might occur.
        cout << "Request DENIED. Not enough resources. Process " << processId << " must wait." << endl;
        waitingProcesses[resourceId].push_back(processId);

        // Since we just made a process wait, it's the perfect time to check if we created a deadlock.
        if (detector.hasCycle(*this)) {
            // A deadlock was found! We need to call our recovery agent to fix it.
            recoveryAgent.initiateRecovery(*this);
            // After recovery, resources might have been freed. Let's try the request again.
            return requestResource(processId, resourceId, count);
        }
        return false;
    }
}

// This function handles a process releasing a resource it was holding.
bool ResourceManager::releaseResource(int processId, int resourceId, int count) {
    cout << "Process " << processId << " releases " << count << " of Resource " << resourceId << endl;
    Process* process = findProcessById(processId);
    Resource* resource = findResourceById(resourceId);

    if (!process || !resource) {
        cerr << "Error: Invalid process or resource ID." << endl;
        return false;
    }

    // Make sure the process actually holds the resources it's trying to release.
    if (process->resourcesHeld.count(resourceId) && process->resourcesHeld[resourceId] >= count) {
        // Update the counts for both the process and the now-freed resource.
        process->resourcesHeld[resourceId] -= count;
        if (process->resourcesHeld[resourceId] == 0) {
            process->resourcesHeld.erase(resourceId);
        }
        resource->availableInstances += count;
        cout << "Resource released successfully." << endl;

        // An important step: now that a resource is free, we should check if any
        // processes that were waiting for it can finally proceed.
        if (waitingProcesses.count(resourceId)) {
            auto& waiting_queue = waitingProcesses[resourceId];
            for(auto it = waiting_queue.begin(); it != waiting_queue.end(); ) {
                if(resource->availableInstances >= 1) {
                    int waitingProcessId = *it;
                    it = waiting_queue.erase(it); // Remove it from the waiting list.
                    cout << "Notifying waiting Process " << waitingProcessId << " that resources are now available." << endl;
                    requestResource(waitingProcessId, resourceId, 1); // Let it try its request again.
                } else {
                    ++it;
                }
            }
        }
        return true;
    } else {
        cerr << "Error: Process " << processId << " cannot release resources it does not hold." << endl;
        return false;
    }
}

// This function just prints a nice, clean summary of the entire system's current state.
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
                int resId = pair.first;
                int count = pair.second;
                cout << "  - Holding " << count << " of Resource " << resId << endl;
            }
        }
    }
     cout << "\n--- Waiting Processes ---" << endl;
    bool anyWaiting = false;
    for (const auto& pair : waitingProcesses) {
        if (!pair.second.empty()) {
            anyWaiting = true;
            cout << "Resource " << pair.first << " is awaited by: ";
            for (int procId : pair.second) {
                cout << "P" << procId << " ";
            }
            cout << endl;
        }
    }
    if(!anyWaiting){
        cout << "No processes are currently waiting for resources." << endl;
    }
    cout << "======================================================\n" << endl;
}