#include "../include/RecoveryAgent.h"
#include "../include/ResourceManager.h"
#include <iostream>
#include <algorithm>
#include <vector>
#include <set>
#include <limits>

using namespace std;

// This function is the "emergency response" that gets called when a deadlock is confirmed.
// It's responsible for breaking the cycle by intelligently choosing a process to sacrifice.
void RecoveryAgent::initiateRecovery(ResourceManager& rm) {
    cout << "\nDEADLOCK DETECTED! Initiating intelligent recovery..." << endl;

    // --- OUR CUSTOM OPTIMIZATION: Smarter Victim Selection ---
    // Instead of just picking a random victim, we'll find the one that's "least costly"
    // to terminate, causing the least disruption to the whole system.

    // Step 1: First, we need a clean list of all the processes that are actually stuck in the deadlock.
    set<int> deadlockedProcesses;
    for (const auto& pair : rm.waitingProcesses) {
        for (int waitingId : pair.second) {
            deadlockedProcesses.insert(waitingId);
        }
    }

    if (deadlockedProcesses.empty()) {
        cout << "Recovery failed: Could not identify any deadlocked processes." << endl;
        return;
    }

    // Step 2: Now, for each of those processes, we'll calculate a "cost".
    // Our cost function is: (number of resources held) - (process priority).
    // The idea is that killing a process with few resources and low priority is "cheaper".
    // We want to find the process with the absolute LOWEST cost.
    int victimId = -1;
    double minCost = numeric_limits<double>::max(); // Start with a ridiculously high number.

    cout << "  - Analyzing potential victims..." << endl;
    for (int procId : deadlockedProcesses) {
        Process* p = rm.findProcessById(procId);
        if (p) {
            double cost = p->resourcesHeld.size() - p->priority;
            
            cout << "    - Process " << p->id << " has cost: " << cost 
                      << " (Resources=" << p->resourcesHeld.size() << ", Priority=" << p->priority << ")" << endl;
            
            // If this process is "cheaper" than the best one we've found so far, it becomes our new candidate.
            if (cost < minCost) {
                minCost = cost;
                victimId = p->id;
            }
        }
    }
    
    if (victimId == -1) {
        cout << "Recovery failed: Could not select a victim from the deadlocked set." << endl;
        return;
    }

    cout << "  - Selected Process " << victimId << " as victim (Lowest Cost: " << minCost << ")." << endl;

    // Step 3: We have our victim. Now we take back all of its resources (preemption).
    Process* victimProcess = rm.findProcessById(victimId);
    if (victimProcess) {
        for (const auto& pair : victimProcess->resourcesHeld) {
            int resourceId = pair.first;
            int count = pair.second;
            cout << "  - Preempting " << count << " instance(s) of Resource " << resourceId << " from Process " << victimId << endl;
            rm.findResourceById(resourceId)->availableInstances += count;
        }
        victimProcess->resourcesHeld.clear(); // The victim now holds nothing.

        // Finally, we clean up by removing the victim from any waiting lists it was on.
        for (auto& pair : rm.waitingProcesses) {
            auto& waiting_procs = pair.second;
            waiting_procs.erase(remove(waiting_procs.begin(), waiting_procs.end(), victimId), waiting_procs.end());
        }

        cout << "Recovery complete. The deadlock cycle is broken." << endl;
    }
}