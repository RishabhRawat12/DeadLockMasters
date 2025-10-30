#pragma once

#include <string>

using namespace std;

// Represents a type of system resource.
class Resource
{
public:
    int id;
    int totalInstances;
    int availableInstances;

    Resource(int resourceId, int totalInstances);
};