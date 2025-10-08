#include "include/ResourceManager.h"
#include <iostream>
#include <fstream>
#include <sstream>
#include <string>
#include <vector>

using namespace std;

// The main entry point of our program.
// It's responsible for reading a scenario file and driving the simulation.
int main() {
    cout << "Deadlock Master Dynamic Simulation Starting..." << endl;

    ResourceManager rm;
    string scenarioFilePath = "scenario.txt";
    ifstream scenarioFile(scenarioFilePath);

    // First, make sure we can actually open the scenario file.
    if (!scenarioFile.is_open()) {
        cerr << "Error: Could not open scenario file: " << scenarioFilePath << endl;
        return 1; // Exit with an error code.
    }

    cout << ">>> Reading scenario from " << scenarioFilePath << endl;

    string line;
    int lineNumber = 0;
    bool definitions_done = false;
    // Read the file line by line.
    while (getline(scenarioFile, line)) {
        lineNumber++;
        // Ignore any lines that are empty or are comments (start with #).
        if (line.empty() || line[0] == '#') {
            continue;
        }

        stringstream ss(line);
        char type;
        ss >> type;

        // Parse the line based on its first character (P, R, or E).
        if (type == 'P') {
            int processId;
            ss >> processId;
            rm.addProcess(Process(processId));
            cout << "  - Defined Process " << processId << endl;
        } else if (type == 'R') {
            int resourceId, instances;
            ss >> resourceId >> instances;
            rm.addResource(Resource(resourceId, instances));
            cout << "  - Defined Resource " << resourceId << " with " << instances << " instances" << endl;
        } else if (type == 'E') {
            // Once we see the first Event, we know the definitions are done.
            // Let's print the initial state of the system for clarity.
            if(!definitions_done){
                cout << endl;
                rm.printState();
                definitions_done = true;
            }
            
            int processId, resourceId, count;
            string action;
            ss >> processId >> action >> resourceId >> count;

            if (action == "REQUEST") {
                rm.requestResource(processId, resourceId, count);
            } else if (action == "RELEASE") {
                rm.releaseResource(processId, resourceId, count);
            } else {
                cerr << "Warning: Unknown action '" << action << "' on line " << lineNumber << endl;
            }
        }
    }

    cout << "\n>>> Final system state after scenario execution:" << endl;
    rm.printState();

    cout << "The 'Arbitrator' system has concluded the simulation." << endl;

    return 0;
}