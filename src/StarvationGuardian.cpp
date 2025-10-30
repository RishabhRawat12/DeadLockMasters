#include "../include/StarvationGuardian.h"
#include "../include/ResourceManager.h"
#include <iostream>
#include <chrono>
#include <set>
#include <string>

using namespace std;

// Apply Aging.
void StarvationGuardian::applyAging(ResourceManager &rm)
{
    long long currentTime = chrono::duration_cast<chrono::seconds>(
                                chrono::system_clock::now().time_since_epoch())
                                .count();

    // Get all waiting process IDs.
    set<int> currentlyWaitingIds;
    for (const auto &pair : rm.waitingProcesses)
    {
        for (const auto &waitingInfo : pair.second)
        {
            currentlyWaitingIds.insert(waitingInfo.processId);
        }
    }

    // Check each process.
    for (auto &process : rm.processes)
    {
        bool isWaiting = (currentlyWaitingIds.count(process.id) > 0);

        if (isWaiting)
        {
            if (process.waitStartTime == 0)
            {
                // Start wait timer.
                process.waitStartTime = currentTime;
                rm.log("  - Aging: P" + to_string(process.id) + " started waiting.");
            }
            else
            {
                // Check timer.
                long long waitDuration = currentTime - process.waitStartTime;
                const long long AGING_THRESHOLD = 5; // 5 second threshold.
                if (waitDuration > AGING_THRESHOLD)
                {
                    process.increasePriority();
                    rm.log("*** Aging: Increased P" + to_string(process.id) + " priority to " + to_string(process.priority) + " ***");
                    process.waitStartTime = currentTime; // Reset timer.
                }
            }
        }
        else
        {
            // Reset timer if no longer waiting.
            if (process.waitStartTime != 0)
            {
                rm.log("  - Aging: P" + to_string(process.id) + " stopped waiting.");
                process.resetWaitTime();
            }
        }
    }
}