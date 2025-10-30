#include "../include/DeadlockDetector.h"
#include "../include/ResourceManager.h"
#include <iostream>
#include <vector>
#include <numeric>
#include <map>

using namespace std;

// Helper: DFS for cycle detection.
bool dfs_cycle_check(int u, const vector<vector<int>> &adj, vector<int> &visited, vector<int> &recursionStack)
{
    visited[u] = 1;
    recursionStack[u] = 1;

    for (int v : adj[u])
    {
        if (!visited[v])
        {
            if (dfs_cycle_check(v, adj, visited, recursionStack))
                return true;
        }
        else if (recursionStack[v])
        {
            return true; // Cycle.
        }
    }
    recursionStack[u] = 0; // Backtrack.
    return false;
}

// Build wait-for graph and check for cycles.
bool DeadlockDetector::hasCycle(ResourceManager &rm)
{
    if (rm.processes.empty())
        return false;

    int maxProcessId = 0;
    for (const auto &p : rm.processes)
    {
        if (p.id > maxProcessId)
            maxProcessId = p.id;
    }

    vector<vector<int>> adj(maxProcessId + 1);
    vector<bool> processExists(maxProcessId + 1, false);
    for (const auto &p : rm.processes)
        processExists[p.id] = true;

    // Build graph (waiter -> holder).
    for (const auto &pair : rm.waitingProcesses)
    {
        int resourceId = pair.first;
        vector<int> holders;
        for (const auto &p : rm.processes)
        {
            if (p.resourcesHeld.count(resourceId) && p.resourcesHeld.at(resourceId) > 0)
            {
                holders.push_back(p.id);
            }
        }
        for (const auto &waitingInfo : pair.second)
        {
            int waiterId = waitingInfo.processId;
            if (waiterId >= 0 && waiterId <= maxProcessId)
            {
                for (int holderId : holders)
                {
                    if (holderId >= 0 && holderId <= maxProcessId)
                    {
                        adj[waiterId].push_back(holderId);
                    }
                }
            }
        }
    }

    // Run DFS.
    vector<int> visited(maxProcessId + 1, 0);
    vector<int> recursionStack(maxProcessId + 1, 0);
    for (int i = 0; i <= maxProcessId; ++i)
    {
        if (processExists[i] && !visited[i])
        {
            if (dfs_cycle_check(i, adj, visited, recursionStack))
                return true;
        }
    }
    return false;
}

// Banker's Algorithm: Check if state is safe.
bool DeadlockDetector::isSafeState(ResourceManager &rm)
{
    int n = rm.processes.size();
    int m = rm.resources.size();
    if (n == 0)
        return true;

    // --- Setup Matrices ---
    vector<int> available(m);
    map<int, int> resourceIdToIndex;
    for (int i = 0; i < m; ++i)
    {
        available[i] = rm.resources[i].availableInstances;
        resourceIdToIndex[rm.resources[i].id] = i;
    }

    vector<vector<int>> max_need(n, vector<int>(m, 0));
    vector<vector<int>> allocation(n, vector<int>(m, 0));
    vector<vector<int>> need(n, vector<int>(m, 0));

    map<int, int> processIdToIndex;
    for (int i = 0; i < n; ++i)
    {
        processIdToIndex[rm.processes[i].id] = i;
        for (const auto &pair : rm.processes[i].maxResourcesNeeded)
        {
            if (resourceIdToIndex.count(pair.first))
            {
                max_need[i][resourceIdToIndex[pair.first]] = pair.second;
            }
        }
        for (const auto &pair : rm.processes[i].resourcesHeld)
        {
            if (resourceIdToIndex.count(pair.first))
            {
                allocation[i][resourceIdToIndex[pair.first]] = pair.second;
            }
        }
    }

    // Calculate Need = Max - Allocation
    for (int i = 0; i < n; ++i)
    {
        for (int j = 0; j < m; ++j)
        {
            need[i][j] = max_need[i][j] - allocation[i][j];
            if (need[i][j] < 0)
            {
                rm.log("Error: P" + to_string(rm.processes[i].id) + " alloc > max need.");
                return false;
            }
        }
    }

    // --- Safety Algorithm ---
    vector<bool> finish(n, false);
    vector<int> work = available;
    vector<int> safeSequence;
    int finishedCount = 0;

    while (finishedCount < n)
    {
        bool foundProcess = false;
        for (int i = 0; i < n; ++i)
        {
            if (!finish[i])
            {
                // Check if Need <= Work.
                bool canSatisfyNeed = true;
                for (int j = 0; j < m; ++j)
                {
                    if (need[i][j] > work[j])
                    {
                        canSatisfyNeed = false;
                        break;
                    }
                }
                // If yes, simulate completion.
                if (canSatisfyNeed)
                {
                    for (int j = 0; j < m; ++j)
                        work[j] += allocation[i][j];
                    finish[i] = true;
                    safeSequence.push_back(rm.processes[i].id);
                    finishedCount++;
                    foundProcess = true;
                }
            }
        }
        if (!foundProcess)
        {
            rm.log("Banker's: System is NOT SAFE.");
            return false;
        }
    }

    // State is safe.
    string seq_s = "Banker's: System is SAFE. Sequence: ";
    for (size_t i = 0; i < safeSequence.size(); ++i)
    {
        seq_s += "P" + to_string(safeSequence[i]);
        if (i < safeSequence.size() - 1)
            seq_s += " -> ";
    }
    rm.log(seq_s);
    return true;
}