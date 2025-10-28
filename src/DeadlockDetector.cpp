#include "../include/DeadlockDetector.h"
#include "../include/ResourceManager.h"
#include <iostream>
#include <vector>
#include <numeric>
#include <algorithm> // For std::max

using namespace std; // Assuming usage of std namespace

// Recursive helper for DFS cycle check.
bool dfs_cycle_check_recursive(int currentProcessId, vector<vector<int>>& adj, vector<int>& visited, vector<int>& recursionStack) {
    // Use size_t for comparison with vector size to avoid sign comparison warning
    if (currentProcessId < 0 || static_cast<size_t>(currentProcessId) >= visited.size()) {
        cerr << "Warning: DFS attempted with out-of-bounds process ID (" << currentProcessId << ")." << endl;
        return false;
    }

    visited[currentProcessId] = 1;
    recursionStack[currentProcessId] = 1;

    for (int waitedForProcessId : adj[currentProcessId]) {
        // Use size_t for comparison with vector size
        if (waitedForProcessId >= 0 && static_cast<size_t>(waitedForProcessId) < visited.size()) {
            if (!visited[waitedForProcessId]) {
                if (dfs_cycle_check_recursive(waitedForProcessId, adj, visited, recursionStack)) {
                    return true;
                }
            } else if (recursionStack[waitedForProcessId]) {
                return true;
            }
        } else {
             cerr << "Warning: DFS found an out-of-bounds process ID (" << waitedForProcessId << ") in adjacency list for process " << currentProcessId << endl;
        }
    }
    recursionStack[currentProcessId] = 0; // Backtrack
    return false;
}

// Builds wait-for graph and uses DFS to detect cycles (deadlocks).
bool DeadlockDetector::hasCycle(ResourceManager& rm) {
    int maxProcessId = -1;
    for (const auto& p : rm.processes) {
        maxProcessId = max(maxProcessId, p.id);
    }
    // Correctly iterate map of vectors of pairs
    for (const auto& waitPair : rm.waitingProcesses) {
        for (const std::pair<int, int>& waitEntry : waitPair.second) { // Iterate pairs
             maxProcessId = max(maxProcessId, waitEntry.first); // Access .first for ID
        }
    }

    int graphSize = maxProcessId + 1;
    if (graphSize <= 0) return false;

    vector<vector<int>> adj(graphSize);

    // Build the graph
    for (const auto& waitPair : rm.waitingProcesses) {
        int resourceId = waitPair.first;
        // Correct type for the vector of pairs
        const vector<std::pair<int, int>>& waitingProcsInfo = waitPair.second;

        vector<int> holdingProcIds;
        for (const auto& p : rm.processes) {
            auto heldIt = p.resourcesHeld.find(resourceId);
            if (heldIt != p.resourcesHeld.end() && heldIt->second > 0) {
                 if(p.id >= 0 && p.id < graphSize) {
                    holdingProcIds.push_back(p.id);
                 }
            }
        }

        // Iterate through pairs to get waiting process IDs
        for (const std::pair<int, int>& waitEntry : waitingProcsInfo) {
             int waitingProcId = waitEntry.first; // Get ID from pair
             if(waitingProcId >= 0 && waitingProcId < graphSize) {
                for (int holdingProcId : holdingProcIds) {
                    if (holdingProcId != waitingProcId) {
                       adj[waitingProcId].push_back(holdingProcId);
                    }
                }
             }
        }
    }

    // Run DFS cycle check
    vector<int> visited(graphSize, 0);
    vector<int> recursionStack(graphSize, 0);

    for (const auto& p : rm.processes) {
         if (p.id >= 0 && p.id < graphSize && !visited[p.id]) {
            if (dfs_cycle_check_recursive(p.id, adj, visited, recursionStack)) {
                return true;
            }
         }
    }
    // Also check starting from waiting processes
    for (const auto& waitPair : rm.waitingProcesses) {
        for (const std::pair<int, int>& waitEntry : waitPair.second) { // Iterate pairs
             int waitingId = waitEntry.first; // Get ID from pair
             if (waitingId >= 0 && waitingId < graphSize && !visited[waitingId]) {
                 if (dfs_cycle_check_recursive(waitingId, adj, visited, recursionStack)) {
                     return true;
                 }
             }
        }
    }

    return false; // No cycle found
}


// --- Banker's Algorithm ---
// NOTE: Remains a placeholder implementation.
bool DeadlockDetector::isSafeState(ResourceManager& rm) {
    int numProcesses = rm.processes.size();
    int numResources = rm.resources.size();

    if (numProcesses == 0) return true;

    vector<int> work(numResources);
    for (int j = 0; j < numResources; ++j) {
         Resource* r = rm.findResourceById(j);
         if (r) { work[j] = r->availableInstances; }
         else {
             cerr << "Warning: Banker's - Resource ID " << j << " not found." << endl; work[j] = 0;
         }
    }

    vector<bool> finish(numProcesses, false);
    vector<vector<int>> need(numProcesses, vector<int>(numResources));
    vector<vector<int>> allocation(numProcesses, vector<int>(numResources, 0));

    for (int i = 0; i < numProcesses; ++i) {
        Process* p = rm.findProcessById(i);
        if (!p) {
             cerr << "Warning: Banker's - Process ID " << i << " not found." << endl;
             finish[i] = true;
             continue;
        }
        for (int j = 0; j < numResources; ++j) {
            allocation[i][j] = p->resourcesHeld.count(j) ? p->resourcesHeld.at(j) : 0;
            int max_needed = p->maxResourcesNeeded.count(j) ? p->maxResourcesNeeded.at(j) : 0;
            if (max_needed < allocation[i][j]) {
                 cerr << "Error: Banker's - Process " << p->id << " Max < Allocated for R" << j << "." << endl;
                 return false;
            }
            need[i][j] = max_needed - allocation[i][j];
        }
    }

    int safeSequenceCount = 0;
    vector<int> safeSequence;
    while(safeSequenceCount < numProcesses) {
        bool foundSafeProcess = false;
        for (int i = 0; i < numProcesses; ++i) {
            if (!finish[i]) {
                bool canSatisfyNeed = true;
                for (int j = 0; j < numResources; ++j) {
                    if (need[i][j] > work[j]) {
                        canSatisfyNeed = false;
                        break;
                    }
                }

                if (canSatisfyNeed) {
                    for (int j = 0; j < numResources; ++j) {
                        work[j] += allocation[i][j];
                    }
                    finish[i] = true;
                    foundSafeProcess = true;
                    safeSequenceCount++;
                    safeSequence.push_back(i);
                    break;
                }
            }
        }
        if (!foundSafeProcess) {
            bool all_skipped = true;
            for(int i=0; i<numProcesses; ++i) {
                if (!finish[i]) { all_skipped = false; break; }
            }
            if(all_skipped) break;

            cout << "Banker's Algorithm: Unsafe state detected." << endl;
            return false;
        }
    }

    for(int i=0; i<numProcesses; ++i){
        if(!finish[i] && rm.findProcessById(i)){
             cout << "Banker's Algorithm: Unsafe state (Process " << i << " could not finish)." << endl;
             return false;
        }
    }

    cout << "Banker's Algorithm: Safe state confirmed." << endl;
    return true;
}