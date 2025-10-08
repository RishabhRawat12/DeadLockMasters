#include "../include/Resource.h"

using namespace std;

// When we create a resource, we need to know its unique ID and how many
// total instances of it exist in our system (e.g., we have 2 printers).
// Initially, all of these instances are available.
Resource::Resource(int resourceId, int totalInstances)
    : id(resourceId), totalInstances(totalInstances), availableInstances(totalInstances) {}