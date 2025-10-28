#include "../include/ResourceManager.h"
#include <iostream>
#include <vector>
#include <map>
#include <algorithm> // For std::sort, std::remove_if
#include <utility>   // For std::pair, std::make_pair

using namespace std;

ResourceManager::ResourceManager() {}

void ResourceManager::addProcess(const Process& p) { processes.push_back(p); }
void ResourceManager::addResource(const Resource& r) { resources.push_back(r); }

// Helper to find a process by ID.
Process* ResourceManager::findProcessById(int processId) {
    for (auto& p : processes) if (p.id == processId) return &p;
    return nullptr;
}

// Helper to find a resource by ID.
Resource* ResourceManager::findResourceById(int resourceId) {
    for (auto& r : resources) if (r.id == resourceId) return &r;
    return nullptr;
}

// Handles a process's request for resources.
bool ResourceManager::requestResource(int processId, int resourceId, int count) {
    cout << "Process " << processId << " requests " << count << " of Resource " << resourceId << endl;
    Process* process = findProcessById(processId);
    Resource* resource = findResourceById(resourceId);

    // Basic Input Validation
    if (count <= 0) {
        cerr << "Warning: P" << processId << " requested non-positive count (" << count << ") for R" << resourceId << ". Ignored." << endl; return false;
    }
    if (!process || !resource) {
        cerr << "Error: Invalid P" << processId << " or R" << resourceId << " during request." << endl; return false;
    }

    if (resource->availableInstances >= count) {
        // Grant the request
        resource->availableInstances -= count;
        process->resourcesHeld[resourceId] += count; // Creates entry if needed
        cout << "Request GRANTED." << endl;
        process->resetWaitTime();

        // Remove from waiting queue if present (using correct type std::pair<int, int>)
        if (waitingProcesses.count(resourceId)) {
             auto& queue = waitingProcesses[resourceId];
             // Correct lambda signature for remove_if with std::pair
             queue.erase(std::remove_if(queue.begin(), queue.end(),
                                        [processId](const std::pair<int, int>& item){ return item.first == processId; }),
                         queue.end());
             if (queue.empty()) waitingProcesses.erase(resourceId);
        }

        starvationGuardian.applyAging(*this); // Check aging after state change
        return true;
    } else {
        // Deny request, process must wait
        cout << "Request DENIED. Not enough resources. P" << processId << " must wait." << endl;
        // Correct push_back for std::vector<std::pair<int, int>>
        waitingProcesses[resourceId].push_back(std::make_pair(processId, count));
        starvationGuardian.applyAging(*this); // Apply aging check

        // Check for deadlock
        if (detector.hasCycle(*this)) {
            recoveryAgent.initiateRecovery(*this);
            cout << "Recovery attempted. P" << processId << " remains waiting if not preempted." << endl;
        }
        return false; // Request denied
    }
}

// Handles a process releasing resources.
bool ResourceManager::releaseResource(int processId, int resourceId, int count) {
    cout << "Process " << processId << " releases " << count << " of Resource " << resourceId << endl;
    Process* process = findProcessById(processId);
    Resource* resource = findResourceById(resourceId);

    // Basic Input Validation
    if (count <= 0) {
        cerr << "Warning: P" << processId << " tried release non-positive count (" << count << ") for R" << resourceId << ". Ignored." << endl; return false;
    }
    if (!process || !resource) {
        cerr << "Error: Invalid P" << processId << " or R" << resourceId << " during release." << endl; return false;
    }

    // Check if process holds enough resources
    if (process->resourcesHeld.count(resourceId) && process->resourcesHeld.at(resourceId) >= count) {
        process->resourcesHeld.at(resourceId) -= count;
        if (process->resourcesHeld.at(resourceId) == 0) process->resourcesHeld.erase(resourceId);
        resource->availableInstances += count;
        cout << "Resource released. Available R" << resourceId << ": " << resource->availableInstances << endl;

        // Notify waiting processes, sorted by priority
        if (waitingProcesses.count(resourceId) && !waitingProcesses.at(resourceId).empty()) {
            auto& waiting_queue = waitingProcesses.at(resourceId); // vector<pair<int, int>>

            // Sort by priority (desc) using correct type std::pair<int, int>
            std::sort(waiting_queue.begin(), waiting_queue.end(),
                      [this](const std::pair<int, int>& a, const std::pair<int, int>& b) {
                          Process* procA = findProcessById(a.first);
                          Process* procB = findProcessById(b.first);
                          if (!procA) return false;
                          if (!procB) return true;
                          return procA->priority > procB->priority; // Higher priority first
                      });

            cout << "Notifying waiting processes for R" << resourceId << " (priority order)..." << endl;
            auto it = waiting_queue.begin();
            while (it != waiting_queue.end()) {
                // Correct access using ->first and ->second for iterators to std::pair
                int waitingProcessId = it->first;
                int requestedCount = it->second;
                Process* waitingProcess = findProcessById(waitingProcessId);

                if (waitingProcess && resource->availableInstances >= requestedCount) {
                    cout << "Attempting grant for P" << waitingProcessId << " (Need: " << requestedCount
                         << ", Prio: " << waitingProcess->priority << ")..." << endl;
                    it = waiting_queue.erase(it); // Remove *before* recursive call
                    requestResource(waitingProcessId, resourceId, requestedCount);
                    // Iterator 'it' is already advanced by erase
                } else {
                    if (!waitingProcess) {
                         cerr << "Warning: Waiting P" << waitingProcessId << " not found. Removing." << endl;
                         it = waiting_queue.erase(it);
                    } else {
                         cout << "Insufficient resources (" << resource->availableInstances << ") for P"
                              << waitingProcessId << " (needs " << requestedCount << "). Stopping notifications." << endl;
                        break; // Stop checking lower priority
                    }
                }
            }
            if (waiting_queue.empty()) waitingProcesses.erase(resourceId);
        }

        starvationGuardian.applyAging(*this); // Check aging after state change
        return true;
    } else {
        cerr << "Error: P" << processId << " cannot release " << count << " of R" << resourceId
             << ". Holds " << (process->resourcesHeld.count(resourceId) ? process->resourcesHeld.at(resourceId) : 0) << "." << endl;
        return false;
    }
}

// Prints a summary of the current system state.
void ResourceManager::printState() {
    cout << "\n--- SYSTEM STATE ---" << endl;
    cout << "Resources:" << endl;
    for (const auto& r : resources) cout << "  R" << r.id << ": Avail=" << r.availableInstances << "/" << r.totalInstances << endl;

    cout << "Processes:" << endl;
    for (const auto& p : processes) {
        cout << "  P" << p.id << " (Prio:" << p.priority << ") Holding: {";
        bool first = true;
        for (const auto& pair : p.resourcesHeld) {
             if(pair.second > 0) { // Only show held resources > 0
                if (!first) { cout << ", "; }
                cout << "R" << pair.first << ":" << pair.second; // Corrected indentation warning
                first = false;
             }
        }
        if (first) cout << "None";
        cout << "} MaxNeed: {";
        first = true;
        for (const auto& pair : p.maxResourcesNeeded) {
           if (!first) { cout << ", "; }
           cout << "R" << pair.first << ":" << pair.second; // Corrected indentation warning
           first = false;
        }
        if (first) cout << "N/A";
       cout << "}" << endl;
    }

    cout << "Waiting Processes:" << endl;
    bool anyWaiting = false;
    for (const auto& pair : waitingProcesses) { // pair is <int, vector<pair<int,int>>>
        if (!pair.second.empty()) {
            anyWaiting = true;
            cout << "  R" << pair.first << ": ";
            bool first = true;
            for (const auto& waitInfo : pair.second) { // waitInfo is std::pair<int, int>
                if (!first) cout << ", ";
                // Correct access using .first and .second for std::pair
                cout << "P" << waitInfo.first << "(needs " << waitInfo.second << ")";
                first = false;
            }
            cout << endl;
        }
    }
    if (!anyWaiting) cout << "  None" << endl;
    cout << "---------------------\n" << endl;
}