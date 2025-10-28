#include "../include/StarvationGuardian.h"
#include "../include/ResourceManager.h"
#include <iostream>
#include <chrono>
#include <utility> // Need this for std::pair

using namespace std;
using namespace std::chrono;

// Constructor
StarvationGuardian::StarvationGuardian(long long threshold) : agingThresholdSeconds(threshold) {}

// Setter for threshold
void StarvationGuardian::setAgingThreshold(long long threshold) {
    if (threshold > 0) {
        agingThresholdSeconds = threshold;
    } else {
        cerr << "Warning: Aging threshold must be positive. Keeping current value: " << agingThresholdSeconds << endl;
    }
}

// Applies Aging technique.
void StarvationGuardian::applyAging(ResourceManager& rm) {
    long long currentTime = duration_cast<seconds>(system_clock::now().time_since_epoch()).count();

    for (auto& process : rm.processes) {
        bool isWaiting = false;
        // Check if process ID exists in any waiting list
        for (const auto& map_pair : rm.waitingProcesses) { // Use different name
            for (const std::pair<int, int>& waitEntry : map_pair.second) { // Iterate through pairs
                if (process.id == waitEntry.first) {    // Access ID from pair
                    isWaiting = true;
                    break;
                }
            }
            if (isWaiting) break;
        }

        if (isWaiting) {
            if (process.waitStartTime == 0) {
                process.waitStartTime = currentTime; // Start timer if not already waiting
            } else {
                if (currentTime - process.waitStartTime > agingThresholdSeconds) {
                    process.increasePriority();
                    cout << "Starvation prevention: Increased priority of Process " << process.id
                         << " to " << process.priority << " (waited > " << agingThresholdSeconds << "s)" << endl;
                    process.waitStartTime = currentTime; // Reset timer after boost
                }
            }
        } else {
            // Reset timer if process is no longer waiting
            if (process.waitStartTime != 0) {
                 process.resetWaitTime();
            }
        }
    }
}