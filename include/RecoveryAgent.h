#ifndef RECOVERYAGENT_H
#define RECOVERYAGENT_H

// Forward declaration
class ResourceManager;

// Responsible for resolving detected deadlocks.
class RecoveryAgent {
public:
    // Initiates the deadlock recovery process (victim selection, preemption).
    void initiateRecovery(ResourceManager& rm);
};

#endif // RECOVERYAGENT_H