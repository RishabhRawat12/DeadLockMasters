#include "../include/Resource.h"

using namespace std;

// Resource constructor.
Resource::Resource(int resourceId, int totalInstances)
    : id(resourceId), totalInstances(totalInstances), availableInstances(totalInstances) {}