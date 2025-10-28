#include "../include/Resource.h"

// Constructor: Initialize resource ID and instance counts.
Resource::Resource(int resourceId, int totalInstances)
    : id(resourceId), totalInstances(totalInstances), availableInstances(totalInstances) {}