#include "../include/RecoveryAgent.h"
#include "../include/ResourceManager.h"
#include <iostream>
#include <vector>    // Needed for vector operations
#include <set>       // Needed for finding unique deadlocked processes
#include <limits>    // Needed for numeric_limits
#include <algorithm> // Needed for std::remove_if

using namespace std;

// Initiates recovery when a deadlock is detected.
void RecoveryAgent::initiateRecovery(ResourceManager& rm) {
    cout << "\nDEADLOCK DETECTED! Initiating recovery..." << endl;

    // Identify processes involved in the deadlock.
    set<int> deadlockedProcesses;
    for (const auto& pair : rm.waitingProcesses) {
        for (const auto& waitInfo : pair.second) { // waitInfo is pair<int, int>
            deadlockedProcesses.insert(waitInfo.first);
        }
    }

    if (deadlockedProcesses.empty()) {
        cout << "Recovery Error: Could not identify deadlocked processes." << endl; return;
    }

    // Select victim based on lowest cost: (resources held size) - priority.
    int victimId = -1;
    double minCost = numeric_limits<double>::max();
    cout << "  Analyzing potential victims..." << endl;
    for (int procId : deadlockedProcesses) {
        Process* p = rm.findProcessById(procId);
        if (p) {
            // CORRECTED: Use -> to access members of pointer p
            double cost = static_cast<double>(p->resourcesHeld.size()) - p->priority;
            cout << "    - P" << p->id << " cost: " << cost
                 << " (Res=" << p->resourcesHeld.size() << ", Prio=" << p->priority << ")" << endl;
            if (cost < minCost) {
                minCost = cost;
                victimId = p->id;
            }
        }
    }

    if (victimId == -1) {
        cout << "Recovery Error: Could not select a victim." << endl; return;
    }
    cout << "  Selected Victim: P" << victimId << " (Lowest Cost: " << minCost << ")." << endl;

    // Preempt resources from the victim.
    Process* victimProcess = rm.findProcessById(victimId);
    if (victimProcess) {
        // Create a copy of held resource IDs to iterate over safely.
        vector<int> heldResourceIds;
        for(const auto& pair : victimProcess->resourcesHeld) {
            if (pair.second > 0) heldResourceIds.push_back(pair.first); // Only consider resources actually held
        }

        for (int resourceId : heldResourceIds) {
            // Check again if the victim still holds the resource before releasing.
            if (victimProcess->resourcesHeld.count(resourceId)) {
                int count = victimProcess->resourcesHeld.at(resourceId);
                 if (count > 0) {
                    cout << "  Preempting " << count << " of R" << resourceId << " from P" << victimId << endl;
                    // Use releaseResource for correct state updates and notifications.
                    // Force preemption means we don't care about the return value.
                    rm.releaseResource(victimId, resourceId, count);
                 }
                 // Even if releaseResource failed (shouldn't if count > 0),
                 // ensure the map entry is cleaned up if count became 0 somehow.
                 if (victimProcess->resourcesHeld.count(resourceId) && victimProcess->resourcesHeld.at(resourceId) == 0) {
                      victimProcess->resourcesHeld.erase(resourceId);
                 }
            }
        }
        // Final explicit clear for safety, although releaseResource should handle it.
        victimProcess->resourcesHeld.clear();

        // Remove victim from all waiting queues.
        for (auto& pair : rm.waitingProcesses) { // pair is <int, vector<pair<int, int>>>
            auto& waiting_procs = pair.second;
            waiting_procs.erase(remove_if(waiting_procs.begin(), waiting_procs.end(),
                                          [victimId](const std::pair<int, int>& item){ return item.first == victimId; }),
                                waiting_procs.end());
        }
        // Clean up empty resource entries in waitingProcesses map.
        for (auto it = rm.waitingProcesses.begin(); it != rm.waitingProcesses.end(); ) {
             if (it->second.empty()) it = rm.waitingProcesses.erase(it);
             else ++it;
        }

        cout << "Recovery complete. Deadlock cycle broken." << endl;
    } else {
         cout << "Recovery Error: Selected victim P" << victimId << " not found." << endl;
    }
}