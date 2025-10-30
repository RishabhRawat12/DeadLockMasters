#include "../include/RecoveryAgent.h"
#include "../include/ResourceManager.h"
#include <iostream>
#include <algorithm>
#include <vector>
#include <set>
#include <limits>
#include <map>

using namespace std;

Process *RecoveryAgent::getVictimProcess()
{
    return lastVictimProcess;
}
map<int, int> RecoveryAgent::getPreemptedResources()
{
    return lastVictimPreemptedResources;
}

// Attempt deadlock recovery.
bool RecoveryAgent::initiateRecovery(ResourceManager &rm)
{
    rm.log("\nDEADLOCK DETECTED! Initiating recovery...");
    lastVictimProcess = nullptr;
    lastVictimPreemptedResources.clear();

    // 1. Identify potential cycle members.
    set<int> potentialCycleMembers;
    for (const auto &pair : rm.waitingProcesses)
    {
        for (const auto &waitingInfo : pair.second)
        {
            potentialCycleMembers.insert(waitingInfo.processId); // Waiter.
            Resource *res = rm.findResourceById(pair.first);
            if (res)
            { // Holders.
                for (const auto &p : rm.processes)
                {
                    if (p.resourcesHeld.count(pair.first) && p.resourcesHeld.at(pair.first) > 0)
                    {
                        potentialCycleMembers.insert(p.id);
                    }
                }
            }
        }
    }

    if (potentialCycleMembers.empty())
    {
        rm.log("*** Recovery FAILED: Cannot identify involved processes. ***");
        return false;
    }

    // 2. Select victim with lowest cost.
    int victimId = -1;
    double minCost = numeric_limits<double>::max();

    rm.log("  - Analyzing potential victims...");
    for (int procId : potentialCycleMembers)
    {
        Process *p = rm.findProcessById(procId);
        if (p)
        {
            // Cost = (types held) + (instances held) - priority.
            double resourceCost = p->resourcesHeld.size();
            for (const auto &pair : p->resourcesHeld)
            {
                resourceCost += pair.second;
            }
            // CHANGED: Fixed typo p.priority to p->priority
            double cost = resourceCost - p->priority;

            rm.log("    - P" + to_string(p->id) + " cost: " + to_string(cost));
            if (cost < minCost)
            {
                minCost = cost;
                victimId = p->id;
            }
        }
    }

    if (victimId == -1)
    {
        rm.log("*** Recovery FAILED: Cannot select victim. ***");
        return false;
    }

    // 3. Preempt victim's resources.
    Process *victimProcessPtr = rm.findProcessById(victimId);
    if (!victimProcessPtr)
    {
        rm.log("*** Recovery FAILED: Victim P" + to_string(victimId) + " not found. ***");
        return false;
    }
    lastVictimProcess = victimProcessPtr;
    rm.log("  - Selected P" + to_string(victimId) + " as victim (Cost: " + to_string(minCost) + ").");
    lastVictimPreemptedResources = victimProcessPtr->resourcesHeld;

    for (const auto &pair : lastVictimPreemptedResources)
    {
        Resource *res = rm.findResourceById(pair.first);
        if (res)
        {
            rm.log("  - Preempting " + to_string(pair.second) + " of R" + to_string(pair.first) + " from P" + to_string(victimId));
            res->availableInstances += pair.second;
        }
    }
    victimProcessPtr->resourcesHeld.clear();
    victimProcessPtr->resetWaitTime();

    // 4. Remove victim from waiting lists.
    rm.log("  - Removing P" + to_string(victimId) + " from wait lists.");
    for (auto &pair : rm.waitingProcesses)
    {
        auto &waiting_list = pair.second;
        waiting_list.remove_if([victimId](const WaitingInfo &info)
                               { return info.processId == victimId; });
    }

    rm.log("Recovery successful for P" + to_string(victimId) + ".");
    return true;
}