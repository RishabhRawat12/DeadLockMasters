#include "../include/StarvationGuardian.h"
#include "../include/ResourceManager.h"
#include <iostream>
#include <chrono> // For time functions
#include <vector> // For vector access
#include <map>    // For map access

using namespace std;

// Applies "Aging" technique to prevent starvation.
void StarvationGuardian::applyAging(ResourceManager& rm) {
    long long currentTime = chrono::duration_cast<chrono::seconds>(chrono::system_clock::now().time_since_epoch()).count();
    const long long STARVATION_THRESHOLD = 3; // Seconds to wait before boosting priority

    for (auto& process : rm.processes) {
        bool isWaiting = false;
        // Check if this process is in any waiting queue
        for (const auto& pair : rm.waitingProcesses) { // pair is <int, vector<pair<int,int>>>
            for (const auto& waitInfo : pair.second) { // waitInfo is pair<int, int>
                if (process.id == waitInfo.first) {    // Access first element of the pair
                    isWaiting = true;
                    break;
                }
            }
            if (isWaiting) break;
        }

        if (isWaiting) {
            if (process.waitStartTime == 0) {
                process.waitStartTime = currentTime; // Start timer
            } else if (currentTime - process.waitStartTime > STARVATION_THRESHOLD) {
                process.increasePriority(); // Boost priority
                cout << "Starvation prevention: Increased priority of P" << process.id << " to " << process.priority << endl;
                process.waitStartTime = currentTime; // Reset timer after boosting
            }
        } else {
            process.resetWaitTime(); // Reset timer if not waiting
            // REMOVED: Resetting priority - let it keep boosted priority until it runs or is preempted.
            // if (process.priority > 0) { // Check against initial priority (0)
            //    process.priority = 0; // Reset priority
            // }
        }
    }
}