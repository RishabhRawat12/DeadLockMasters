#include "../include/Process.h"
#include <chrono>

using namespace std;

// The constructor is pretty simple. When a new process is created,
// we just assign it the ID we're given and initialize its priority and wait timer to zero.
Process::Process(int processId) : id(processId), priority(0), waitStartTime(0) {}

// This is part of our "Aging" technique to prevent starvation.
// If a process waits for too long, we just bump up its priority value.
void Process::increasePriority() {
    this->priority++;
}

// A simple reset function. We call this when a process that was waiting
// finally gets the resources it needed. It's no longer starving, so its timer can be reset.
void Process::resetWaitTime() {
    this->waitStartTime = 0;
}