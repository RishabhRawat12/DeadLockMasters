#include "../include/Process.h"
#include <chrono> // Required for time point (though calculation moved to StarvationGuardian)

using namespace std;

// Constructor: Initialize ID, priority, and wait timer.
Process::Process(int processId) : id(processId), priority(0), waitStartTime(0) {}

// Increase process priority (part of Aging).
void Process::increasePriority() {
    this->priority++;
}

// Reset wait timer (called when resource granted or if not waiting).
void Process::resetWaitTime() {
    this->waitStartTime = 0;
}