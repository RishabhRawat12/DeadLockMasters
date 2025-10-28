#ifndef STARVATIONGUARDIAN_H
#define STARVATIONGUARDIAN_H

// Forward declaration
class ResourceManager;

// Monitors and prevents process starvation.
class StarvationGuardian {
private:
    long long agingThresholdSeconds; // Configurable time threshold for boosting priority

public:
    // Constructor to set the threshold.
    StarvationGuardian(long long threshold = 5); // Default to 5 seconds

    // Applies the Aging technique.
    void applyAging(ResourceManager& rm);

    // Allows changing the threshold dynamically if needed.
    void setAgingThreshold(long long threshold);
};

#endif // STARVATIONGUARDIAN_H