#include "../include/Process.h"

using namespace std;

// Process constructor.
Process::Process(int processId) : id(processId), priority(0), waitStartTime(0) {}

// Increment priority.
void Process::increasePriority()
{
    this->priority++;
}

// Reset wait timer.
void Process::resetWaitTime()
{
    this->waitStartTime = 0;
}