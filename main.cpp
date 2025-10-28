#include "include/ResourceManager.h"
#include <iostream>
#include <fstream>
#include <sstream>
#include <string>
#include <vector>
#include <stdexcept> // For exception handling during parsing

using namespace std;

// Main simulation entry point.
int main() {
    cout << "Deadlock Master Dynamic Simulation Starting..." << endl;

    ResourceManager rm;
    string scenarioFilePath = "scenario.txt";
    ifstream scenarioFile(scenarioFilePath);

    if (!scenarioFile.is_open()) {
        cerr << "Error: Could not open scenario file: " << scenarioFilePath << endl;
        return 1;
    }

    cout << ">>> Reading scenario from " << scenarioFilePath << endl;

    string line;
    int lineNumber = 0;
    bool definitions_done = false;

    while (getline(scenarioFile, line)) {
        lineNumber++;
        // Trim leading/trailing whitespace (optional but good practice)
        line.erase(0, line.find_first_not_of(" \t\n\r\f\v"));
        line.erase(line.find_last_not_of(" \t\n\r\f\v") + 1);


        if (line.empty() || line[0] == '#') {
            continue; // Skip empty lines and comments
        }

        stringstream ss(line);
        char type;
        ss >> type; // Read the first character

        try { // Add try-catch for robust parsing
            if (type == 'P') { // Process Definition
                int processId;
                if (!(ss >> processId) || !ss.eof()) { // Check if read succeeded and no extra chars
                     throw runtime_error("Invalid format for Process definition");
                }
                if (processId < 0) throw runtime_error("Process ID cannot be negative");
                rm.addProcess(Process(processId));
                cout << "  - Defined Process " << processId << endl;

            } else if (type == 'R') { // Resource Definition
                int resourceId, instances;
                 if (!(ss >> resourceId >> instances) || !ss.eof()) {
                     throw runtime_error("Invalid format for Resource definition");
                }
                 if (resourceId < 0) throw runtime_error("Resource ID cannot be negative");
                 if (instances <= 0) throw runtime_error("Resource instances must be positive");
                rm.addResource(Resource(resourceId, instances));
                cout << "  - Defined Resource " << resourceId << " with " << instances << " instances" << endl;

            } else if (type == 'E') { // Event
                if(!definitions_done){
                    cout << endl;
                    rm.printState(); // Print initial state before first event
                    definitions_done = true;
                }

                int processId, resourceId, count;
                string action;
                if (!(ss >> processId >> action >> resourceId >> count) || !ss.eof()) {
                     throw runtime_error("Invalid format for Event definition");
                }
                 if (processId < 0 || resourceId < 0) throw runtime_error("Event IDs cannot be negative");
                 if (count <= 0) throw runtime_error("Event count must be positive");


                if (action == "REQUEST") {
                    rm.requestResource(processId, resourceId, count);
                     // Apply aging after a request might cause waiting
                     rm.starvationGuardian.applyAging(rm);
                } else if (action == "RELEASE") {
                    rm.releaseResource(processId, resourceId, count);
                     // Apply aging after a release might change waiting status
                    rm.starvationGuardian.applyAging(rm);
                } else {
                    cerr << "Warning: Unknown action '" << action << "' on line " << lineNumber << endl;
                }
            } else {
                 cerr << "Warning: Unknown definition/event type '" << type << "' on line " << lineNumber << endl;
            }
        } catch (const exception& e) {
             cerr << "Error parsing line " << lineNumber << ": " << e.what() << " - Line: '" << line << "'" << endl;
             // Decide whether to continue or abort simulation on error
             // return 1; // Abort on error
        }
    } // End while loop

    cout << "\n>>> Final system state after scenario execution:" << endl;
    rm.printState();

    cout << "The 'Arbitrator' system has concluded the simulation." << endl;

    return 0;
}