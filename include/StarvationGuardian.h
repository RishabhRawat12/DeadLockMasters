#pragma once

class ResourceManager;

// This module's job is to prevent starvation. It monitors processes
// that have been waiting for a long time and takes action.
class StarvationGuardian {
public:
    // Implements the "Aging" technique by increasing the priority of long-waiting processes.
    void applyAging(ResourceManager& rm);
};