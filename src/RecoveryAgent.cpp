#include "../include/RecoveryAgent.h"
#include "../include/ResourceManager.h"
#include <iostream>
#include <algorithm> // For std::remove_if
#include <vector>
#include <set>
#include <limits>
#include <utility> // Need this for std::pair

using namespace std; // Assuming usage of std namespace

// Called when a deadlock is detected to break the cycle.
void RecoveryAgent::initiateRecovery(ResourceManager& rm) {
    cout << "\nDEADLOCK DETECTED! Initiating intelligent recovery..." << endl;

    // Step 1: Identify potentially deadlocked processes.
    set<int> deadlockedProcesses;
    for (const auto& map_pair : rm.waitingProcesses) { // Use a different name than 'pair'
        for (const std::pair<int, int>& waitEntry : map_pair.second) { // Iterate through pairs
            deadlockedProcesses.insert(waitEntry.first); // Insert process ID from pair
        }
    }

    if (deadlockedProcesses.empty()) {
        cout << "Recovery Warning: Could not identify any waiting processes during deadlock." << endl;
        return;
    }

    // Step 2: Calculate cost and find minimum cost victim.
    int victimId = -1;
    double minCost = numeric_limits<double>::max();

    cout << "  - Analyzing potential victims..." << endl;
    for (int procId : deadlockedProcesses) {
        Process* p = rm.findProcessById(procId);
        if (p) {
            double cost = static_cast<double>(p->resourcesHeld.size()) - p->priority;
            cout << "    - Process " << p->id << " has cost: " << cost
                      << " (Resources=" << p->resourcesHeld.size() << ", Priority=" << p->priority << ")" << endl;
            if (cost < minCost) {
                minCost = cost;
                victimId = p->id;
            }
        } else {
             cout << "    - Warning: Process " << procId << " (in waiting list) not found in main process list." << endl;
        }
    }

    if (victimId == -1) {
        cout << "Recovery failed: Could not select a victim." << endl;
         if (!deadlockedProcesses.empty()) {
            victimId = *deadlockedProcesses.begin();
             cout << "  - Fallback: Selecting first identified process " << victimId << " as victim." << endl;
        } else {
            return;
        }
    } else {
       cout << "  - Selected Process " << victimId << " as victim (Lowest Cost: " << minCost << ")." << endl;
    }

    // Step 3: Preempt resources.
    Process* victimProcess = rm.findProcessById(victimId);
    if (victimProcess) {
        vector<int> resourceIdsToRelease;
        for (const auto& resource_pair : victimProcess->resourcesHeld) { // Use different name
            resourceIdsToRelease.push_back(resource_pair.first);
        }

        for (int resourceId : resourceIdsToRelease) {
             if (victimProcess->resourcesHeld.count(resourceId)) {
                int count = victimProcess->resourcesHeld[resourceId];
                cout << "  - Preempting " << count << " instance(s) of Resource " << resourceId << " from Process " << victimId << endl;
                Resource* resource = rm.findResourceById(resourceId);
                 if (resource) {
                     resource->availableInstances += count;
                 } else {
                      cerr << "  - Error: Could not find Resource " << resourceId << " to return instances." << endl;
                 }
                 victimProcess->resourcesHeld.erase(resourceId);
             }
        }
       victimProcess->resourcesHeld.clear();

        // Step 4: Remove victim from waiting lists using remove_if with a lambda.
        for (auto& map_pair : rm.waitingProcesses) { // Use different name
            auto& waiting_procs = map_pair.second;
            // Correct lambda signature: takes the element type of the vector
            waiting_procs.erase(
                remove_if(waiting_procs.begin(), waiting_procs.end(),
                          [victimId](const std::pair<int, int>& p){ return p.first == victimId; }), // Correct type here
                waiting_procs.end()
            );
        }
        victimProcess->resetWaitTime();

        // Notify potentially unblocked processes
        rm.notifyWaitingProcesses();

        cout << "Recovery complete. The deadlock cycle is broken." << endl;
    } else {
         cout << "Recovery Error: Selected victim Process " << victimId << " not found." << endl;
    }
}