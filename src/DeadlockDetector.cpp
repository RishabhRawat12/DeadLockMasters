#include "../include/DeadlockDetector.h"
#include "../include/ResourceManager.h" 
#include <iostream>
#include <vector>

using namespace std;

// This is a helper function that uses Depth First Search (DFS) to find a cycle in our wait-for graph.
// It's the classic, textbook way to see if a directed graph has a cycle.
bool dfs_cycle_check(int u, vector<vector<int>>& adj, vector<int>& visited, vector<int>& recursionStack) {
    visited[u] = 1;
    recursionStack[u] = 1; // Mark that this process is currently in our recursion path.

    // Look at all the processes this process is waiting on.
    for (int v : adj[u]) {
        if (!visited[v]) {
            if (dfs_cycle_check(v, adj, visited, recursionStack)) {
                return true; // A cycle was found deeper in the recursion.
            }
        } 
        // If we find a neighbor that is *already* in our current recursion path, we've found a cycle!
        else if (recursionStack[v]) {
            return true;
        }
    }
    recursionStack[u] = 0; // Remove from the path as we backtrack.
    return false;
}

// This function builds a "wait-for" graph from the current system state and then
// uses our DFS helper to check if there's a circular dependency (a deadlock).
bool DeadlockDetector::hasCycle(ResourceManager& rm) {
    int numProcesses = rm.processes.size();
    // 'adj' is an adjacency list. adj[i] will contain a list of processes that process 'i' is waiting for.
    vector<vector<int>> adj(numProcesses);
    
    // Let's build the graph. We go through the list of waiting processes...
    for(const auto& pair : rm.waitingProcesses) {
        int resourceId = pair.first;
        const vector<int>& waitingProcs = pair.second;

        for(int waitingProcId : waitingProcs) {
            // Find which process currently holds the resource that 'waitingProcId' wants.
            for(const auto& p : rm.processes) {
                if(p.resourcesHeld.count(resourceId)) {
                    // We draw a directed edge from the waiting process to the holding process.
                    // This means: "waitingProcId" is waiting for "p.id".
                    adj[waitingProcId].push_back(p.id);
                }
            }
        }
    }

    // Now that the graph is built, we can run our DFS cycle check on it.
    vector<int> visited(numProcesses, 0);
    vector<int> recursionStack(numProcesses, 0);

    for (int i = 0; i < numProcesses; ++i) {
        if (!visited[i]) {
            if (dfs_cycle_check(i, adj, visited, recursionStack)) {
                return true; // Cycle detected! We officially have a deadlock.
            }
        }
    }
    return false; // No cycle was found. The system is safe for now.
}