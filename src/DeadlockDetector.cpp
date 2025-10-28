#include "../include/DeadlockDetector.h"
#include "../include/ResourceManager.h"
#include <iostream>
#include <vector>
#include <map> // Needed for Banker's Algorithm map access

using namespace std;

// Helper: DFS for cycle detection.
bool dfs_cycle_check(int u, vector<vector<int>>& adj, vector<int>& visited, vector<int>& recursionStack) {
    // Cast to size_t for comparison to avoid signed/unsigned warning
    if (static_cast<size_t>(u) >= visited.size()) return false;
    visited[u] = 1;
    recursionStack[u] = 1;

    for (int v : adj[u]) {
        // Cast to size_t for comparison to avoid signed/unsigned warning
        if (static_cast<size_t>(v) >= visited.size()) continue;
        if (!visited[v]) {
            if (dfs_cycle_check(v, adj, visited, recursionStack)) return true;
        } else if (recursionStack[v]) {
            return true; // Cycle detected
        }
    }
    recursionStack[u] = 0;
    return false;
}

// Builds wait-for graph and detects cycles.
bool DeadlockDetector::hasCycle(ResourceManager& rm) {
    int numProcesses = rm.processes.size();
    if (numProcesses == 0) return false;
    vector<vector<int>> adj(numProcesses);

    for(const auto& pair : rm.waitingProcesses) {
        int resourceId = pair.first;
        const auto& waitingInfo = pair.second; // vector<pair<int, int>>

        for(const auto& waitPair : waitingInfo) { // waitPair is std::pair<int, int>
            int waitingProcId = waitPair.first;   // Correct access to pair element
            if (waitingProcId < 0 || waitingProcId >= numProcesses) continue;

            int holderProcId = -1;
            for(const auto& p : rm.processes) {
                 if (p.id < 0 || p.id >= numProcesses) continue;
                if (p.id != waitingProcId && p.resourcesHeld.count(resourceId)) {
                   holderProcId = p.id; break;
                }
            }

            if (holderProcId != -1) {
                 if (holderProcId < 0 || holderProcId >= numProcesses) continue;
                 bool edgeExists = false;
                 for (int neighbor : adj[waitingProcId]) {
                     if (neighbor == holderProcId) { edgeExists = true; break; }
                 }
                 if (!edgeExists) adj[waitingProcId].push_back(holderProcId);
            }
        }
    }

    vector<int> visited(numProcesses, 0);
    vector<int> recursionStack(numProcesses, 0);
    for (int i = 0; i < numProcesses; ++i) {
        if (!visited[i]) {
            if (dfs_cycle_check(i, adj, visited, recursionStack)) return true;
        }
    }
    return false;
}

// Checks safety using Banker's Algorithm.
bool DeadlockDetector::isSafeState(ResourceManager& rm) {
    int numProcesses = rm.processes.size();
    int numResources = rm.resources.size();
    if (numProcesses == 0) return true;

    vector<int> work(numResources);
    for (int j = 0; j < numResources; ++j) {
        Resource* r = rm.findResourceById(j);
        if (!r) { cerr << "Banker's Error: R" << j << " invalid." << endl; return false; }
        work[j] = r->availableInstances;
    }

    vector<bool> finish(numProcesses, false);
    vector<vector<int>> need(numProcesses, vector<int>(numResources));

    for (int i = 0; i < numProcesses; ++i) {
        // Cast to size_t for comparison to avoid signed/unsigned warning
         if (static_cast<size_t>(i) >= rm.processes.size()) { cerr << "Banker's Error: P index " << i << " OOB." << endl; return false; }
        const Process& p = rm.processes[i];
        for (int j = 0; j < numResources; ++j) {
            int maxNeeded = p.maxResourcesNeeded.count(j) ? p.maxResourcesNeeded.at(j) : 0;
            int allocated = p.resourcesHeld.count(j) ? p.resourcesHeld.at(j) : 0;
            need[i][j] = maxNeeded - allocated;
            if (need[i][j] < 0) {
                cerr << "Banker's Error: P" << p.id << " negative need for R" << j << "." << endl; return false;
            }
        }
    }

    int count = 0;
    while (count < numProcesses) {
        bool foundProcessToFinish = false;
        for (int i = 0; i < numProcesses; ++i) {
            if (!finish[i]) {
                bool canSatisfyNeed = true;
                for (int j = 0; j < numResources; ++j) {
                    if (need[i][j] > work[j]) {
                        canSatisfyNeed = false; break;
                    }
                }
                if (canSatisfyNeed) {
                    for (int j = 0; j < numResources; ++j) {
                        int allocated = rm.processes[i].resourcesHeld.count(j) ? rm.processes[i].resourcesHeld.at(j) : 0;
                        work[j] += allocated;
                    }
                    finish[i] = true;
                    foundProcessToFinish = true;
                    count++;
                }
            }
        }
        if (!foundProcessToFinish) return false; // Unsafe state
    }
    return true; // Safe state
}