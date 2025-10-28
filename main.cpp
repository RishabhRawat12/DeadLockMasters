#include "include/ResourceManager.h"
#include <iostream>
#include <fstream>
#include <sstream>
#include <string>
#include <vector>
#include <stdexcept> // For exception handling
#include <windows.h> // Required for Sleep() function

using namespace std;

int main()
{
    cout << "Deadlock Master Simulation Starting..." << endl;

    ResourceManager rm;
    string scenarioFilePath = "scenario.txt";
    ifstream scenarioFile(scenarioFilePath);

    if (!scenarioFile.is_open())
    {
        cerr << "Error: Could not open scenario file: " << scenarioFilePath << endl;
        return 1;
    }
    cout << ">>> Reading scenario: " << scenarioFilePath << endl;

    string line;
    int lineNumber = 0;
    bool definitions_done = false;

    while (getline(scenarioFile, line))
    {
        lineNumber++;
        line.erase(0, line.find_first_not_of(" \t\n\r\f\v")); // Trim whitespace
        line.erase(line.find_last_not_of(" \t\n\r\f\v") + 1);

        if (line.empty() || line[0] == '#')
            continue; // Skip empty/comment lines

        stringstream ss(line);
        char type;
        if (!(ss >> type))
        {
            cerr << "Warning: Skipping malformed line " << lineNumber << ": '" << line << "'" << endl;
            continue;
        }

        try
        {
            if (type == 'P')
            { // Process Definition
                if (definitions_done)
                    throw runtime_error("'P' definition after 'E' started");
                int processId;
                if (!(ss >> processId) || processId < 0)
                    throw runtime_error("Invalid Process ID");
                if (rm.findProcessById(processId) != nullptr)
                    throw runtime_error("Duplicate Process ID");
                rm.addProcess(Process(processId));
                cout << "  Defined P" << processId << endl;
            }
            else if (type == 'R')
            { // Resource Definition
                if (definitions_done)
                    throw runtime_error("'R' definition after 'E' started");
                int resourceId, instances;
                if (!(ss >> resourceId >> instances) || resourceId < 0 || instances <= 0)
                    throw runtime_error("Invalid Resource ID or instance count (must be > 0)");
                if (rm.findResourceById(resourceId) != nullptr)
                    throw runtime_error("Duplicate Resource ID");
                rm.addResource(Resource(resourceId, instances));
                cout << "  Defined R" << resourceId << " (Count: " << instances << ")" << endl;
            }
            else if (type == 'M')
            { // Max Need Definition
                if (definitions_done)
                    throw runtime_error("'M' definition after 'E' started");
                int processId, resourceId, maxCount;
                if (!(ss >> processId >> resourceId >> maxCount) || processId < 0 || resourceId < 0 || maxCount < 0)
                    throw runtime_error("Invalid ID or Max count");
                Process *p = rm.findProcessById(processId);
                Resource *r = rm.findResourceById(resourceId);
                if (!p)
                    throw runtime_error("Process ID in 'M' not defined");
                if (!r)
                    throw runtime_error("Resource ID in 'M' not defined");
                if (maxCount > r->totalInstances)
                {
                    cerr << "Warning: P" << processId << " Max claim (" << maxCount << ") for R" << resourceId
                         << " exceeds total (" << r->totalInstances << ") on line " << lineNumber << "." << endl;
                }
                if (p->maxResourcesNeeded.count(resourceId))
                {
                    cerr << "Warning: Duplicate Max claim for P" << processId << "/R" << resourceId << " on line " << lineNumber << ". Overwriting." << endl;
                }
                p->maxResourcesNeeded[resourceId] = maxCount;
                cout << "  Set Max P" << processId << " R" << resourceId << " = " << maxCount << endl;
            }
            else if (type == 'E')
            { // Event
                if (!definitions_done)
                {
                    // Validate Banker's Data if needed
                    bool banker_ok = true;
                    for (const auto &p : rm.processes)
                        for (const auto &r : rm.resources)
                            if (!p.maxResourcesNeeded.count(r.id))
                                banker_ok = false;
                    if (!banker_ok)
                    {
                        cerr << "Error: Banker's Max claims incomplete. Aborting." << endl;
                        return 1;
                    }

                    cout << "\n>>> Definitions complete. Initial State:" << endl;
                    rm.printState();
                    definitions_done = true;
                }

                // *** ADD DELAY BEFORE PROCESSING EVENT ***
                cout << "--- Pausing 2s ---" << endl;
                Sleep(3000); // Pause for 2000 milliseconds (2 seconds)
                // *** END DELAY ***

                int processId, resourceId, count;
                string action;
                if (!(ss >> processId >> action))
                    throw runtime_error("Invalid Process ID or Action");
                if (rm.findProcessById(processId) == nullptr)
                    throw runtime_error("Process ID in 'E' not defined");

                if (action == "REQUEST" || action == "RELEASE")
                {
                    if (!(ss >> resourceId >> count) || resourceId < 0 || count < 0)
                        throw runtime_error("Invalid Resource ID or Count");
                    if (rm.findResourceById(resourceId) == nullptr)
                        throw runtime_error("Resource ID in 'E' not defined");

                    if (action == "REQUEST")
                        rm.requestResource(processId, resourceId, count);
                    else
                        rm.releaseResource(processId, resourceId, count);
                }
                else
                {
                    cerr << "Warning: Unknown action '" << action << "' on line " << lineNumber << endl;
                }
            }
            else
            {
                throw runtime_error("Unrecognized line type '" + string(1, type) + "'");
            }

            // Check for extra input on the line, ignore comments
            string junk;
            if (ss >> junk)
            {
                if (junk.length() > 0 && junk[0] != '#')
                {
                    cerr << "Warning: Extra input '" << junk << "' on line " << lineNumber << ": '" << line << "'" << endl;
                }
            }
        }
        catch (const runtime_error &e)
        {
            cerr << "Error parsing line " << lineNumber << ": '" << line << "' - " << e.what() << ". Skipping." << endl;
        }
    } // End file reading loop

    // Final output
    if (!definitions_done && lineNumber > 0)
    {
        cerr << "Warning: Definitions found but no Events." << endl;
        cout << "\n>>> Definitions Only. Final State:" << endl;
        rm.printState();
    }
    else if (definitions_done)
    {
        cout << "\n>>> Scenario finished. Final State:" << endl;
        rm.printState();
    }
    else
    {
        cout << ">>> Scenario file empty or only comments." << endl;
    }

    // --- Simulation Summary ---
    cout << "--- SIMULATION SUMMARY ---" << endl;
    cout << "Total Processes: " << rm.processes.size() << endl;
    cout << "Total Resources: " << rm.resources.size() << endl;
    bool stillWaiting = false;
    for (const auto &pair : rm.waitingProcesses)
        if (!pair.second.empty())
            stillWaiting = true;
    cout << "Waiting Processes at End: " << (stillWaiting ? "Yes" : "No") << endl;
    cout << "--------------------------" << endl;

    cout << "\nSimulation concluded." << endl;
    return 0;
}