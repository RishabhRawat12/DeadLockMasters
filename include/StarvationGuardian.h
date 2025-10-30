#pragma once

using namespace std;

// Forward declaration.
class ResourceManager;

// Prevents process starvation using Aging.
class StarvationGuardian
{
public:
    // Check and boost priority of waiting processes.
    void applyAging(ResourceManager &rm);
};