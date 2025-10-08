#pragma once

class ResourceManager;

// This class is responsible for fixing a deadlock once it has been detected.
class RecoveryAgent {
public:
    // This function is called when a deadlock is confirmed. It contains the
    // logic for choosing a victim and preempting resources to break the cycle.
    void initiateRecovery(ResourceManager& rm);
};