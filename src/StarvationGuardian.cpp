#include "../include/StarvationGuardian.h"
#include "../include/ResourceManager.h"
#include <iostream>
#include <chrono>

using namespace std;

// This function implements the "Aging" technique to prevent starvation.
// It acts like a guardian, periodically checking for processes that have been
// waiting for too long and giving them a priority boost.
void StarvationGuardian::applyAging(ResourceManager& rm) {
    long long currentTime = chrono::duration_cast<chrono::seconds>(chrono::system_clock::now().time_since_epoch()).count();

    for (auto& process : rm.processes) {
        // First, let's figure out if this specific process is currently waiting for a resource.
        bool isWaiting = false;
        for (const auto& pair : rm.waitingProcesses) {
            for (int waitingId : pair.second) {
                if (process.id == waitingId) {
                    isWaiting = true;
                    break;
                }
            }
            if (isWaiting) break;
        }

        // If the process is indeed waiting...
        if (isWaiting) {
            // ...and if we haven't started a timer for it yet, let's start one now.
            if (process.waitStartTime == 0) {
                process.waitStartTime = currentTime;
            } else {
                // If the timer has been running for more than our threshold (e.g., 5 seconds)...
                if (currentTime - process.waitStartTime > 5) {
                    // It's time to boost its priority!
                    process.increasePriority();
                    cout << "Starvation prevention: Increased priority of Process " << process.id << " to " << process.priority << endl;
                    // We reset the timer so it doesn't get boosted again immediately on the next check.
                    process.waitStartTime = currentTime;
                }
            }
        } else {
            // If the process is not waiting for anything, make sure its wait timer is reset to zero.
            process.resetWaitTime();
        }
    }
}