#ifndef RESOURCE_H
#define RESOURCE_H

// Represents a type of resource (e.g., Printer, CPU).
class Resource {
public:
    int id;
    int totalInstances;     // Total instances existing in the system.
    int availableInstances; // Instances currently available.

    Resource(int resourceId, int totalInstances);
};

#endif // RESOURCE_H