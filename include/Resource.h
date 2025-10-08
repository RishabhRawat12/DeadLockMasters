#pragma once

// Represents a type of resource in our system (e.g., Printer, CPU, Memory).
class Resource {
public:
    int id;

    // The total number of instances of this resource that exist in the system.
    int totalInstances;

    // The number of instances currently available for allocation.
    int availableInstances;

    // Constructor to create a new resource type.
    Resource(int resourceId, int totalInstances);
};