#include "../include/ResourceManager.h"
#include <iostream>
#include <sstream>
#include <string>
#include <set> // <-- ADDED THIS INCLUDE

using namespace std;

// This new main.cpp is not a simulation runner.
// It is a command-line "engine" that the Python GUI will control.
// It reads commands from cin and prints JSON state and logs to cout.

// Helper to send a simple log message.
void send_log(string message)
{
    cout << "LOG: " << message << endl;
}

// Helper to send an error message.
void send_error(string message)
{
    cout << "ERR: " << message << endl;
}

// Prints the entire system state as JSON for Python to parse.
void printStateAsJson(ResourceManager &rm)
{
    cout << "---STATE_BEGIN---" << endl; // Start delimiter.

    // Resources
    cout << "{\"resources\": [";
    for (size_t i = 0; i < rm.resources.size(); ++i)
    {
        auto &r = rm.resources[i];
        cout << "{\"id\": " << r.id << ", \"total\": " << r.totalInstances
             << ", \"available\": " << r.availableInstances << "}";
        if (i < rm.resources.size() - 1)
            cout << ",";
    }
    cout << "], " << endl; // End resources

    // Processes
    cout << "\"processes\": [";
    for (size_t i = 0; i < rm.processes.size(); ++i)
    {
        auto &p = rm.processes[i];
        cout << "{\"id\": " << p.id << ", \"priority\": " << p.priority;

        // Held resources
        cout << ", \"held\": [";
        bool firstHeld = true;
        // CHANGED: Replaced C++17 structured binding
        for (const auto &pair : p.resourcesHeld)
        {
            if (!firstHeld)
                cout << ", ";
            // CHANGED: Use pair.first and pair.second
            cout << "{\"id\": " << pair.first << ", \"count\": " << pair.second << "}";
            firstHeld = false;
        }
        cout << "]"; // End held

        // Max needs
        cout << ", \"max_need\": [";
        bool firstMax = true;
        // CHANGED: Replaced C++17 structured binding
        for (const auto &pair : p.maxResourcesNeeded)
        {
            if (!firstMax)
                cout << ", ";
            // CHANGED: Use pair.first and pair.second
            cout << "{\"id\": " << pair.first << ", \"count\": " << pair.second << "}";
            firstMax = false;
        }
        cout << "]"; // End max_need

        cout << "}"; // Close process
        if (i < rm.processes.size() - 1)
            cout << ",";
    }
    cout << "], " << endl; // End processes

    // Waiting processes (links for graph)
    cout << "\"waiting\": [";
    bool firstWait = true;
    // CHANGED: Replaced C++17 structured binding
    for (const auto &pair : rm.waitingProcesses)
    {
        int resId = pair.first;
        const auto &list = pair.second;
        for (auto const &info : list)
        {
            if (!firstWait)
                cout << ",";
            // CHANGED: Use resId (from pair.first) and info
            cout << "  {\"process_id\": " << info.processId << ", \"resource_id\": "
                 << resId << ", \"count\": " << info.count << "}";
            firstWait = false;
        }
    }
    cout << "\n], " << endl; // End waiting (added newline for readability)

    // Deadlock cycle (for highlighting)
    cout << "\"deadlock_cycle\": [";
    if (rm.strategy == DeadlockStrategy::DETECT && rm.detector.hasCycle(rm))
    {
        set<int> cycleProcs;
        // CHANGED: Replaced C++17 structured binding
        for (const auto &pair : rm.waitingProcesses)
        {
            // CHANGED: Use pair.second
            for (auto const &info : pair.second)
                cycleProcs.insert(info.processId);
        }
        bool firstCycle = true;
        for (int id : cycleProcs)
        {
            if (!firstCycle)
                cout << ", ";
            cout << id;
            firstCycle = false;
        }
    }
    cout << "], " << endl; // End deadlock_cycle

    // Log messages
    cout << "\"log\": [";
    bool firstLog = true;
    for (const auto &msg : rm.logMessages)
    {
        if (!firstLog)
            cout << ",";
        string escaped_msg = msg;
        size_t pos = 0;
        while ((pos = escaped_msg.find('"', pos)) != string::npos)
        {
            escaped_msg.replace(pos, 1, "\\\"");
            pos += 2;
        }
        cout << "\"" << escaped_msg << "\"";
        firstLog = false;
    }
    rm.logMessages.clear(); // Clear log after sending.
    cout << "]" << endl;    // End log

    cout << "}" << endl;               // End JSON object
    cout << "---STATE_END---" << endl; // End delimiter.
}

int main()
{
    ResourceManager rm;
    string line;

    // Set output to unbuffered.
    setvbuf(stdout, NULL, _IONBF, 0);

    // Main command loop.
    while (getline(cin, line))
    {
        if (line.empty())
            continue;

        stringstream ss(line);
        char type;
        ss >> type;

        try
        {
            if (type == 'S')
            { // Set Strategy
                string strategyName;
                ss >> strategyName;
                if (strategyName == "AVOID")
                {
                    rm.setStrategy(DeadlockStrategy::AVOID);
                }
                else
                {
                    rm.setStrategy(DeadlockStrategy::DETECT);
                }
            }
            else if (type == 'P')
            { // Add Process
                int pId;
                if (!(ss >> pId))
                {
                    send_error("Invalid Process ID");
                    continue;
                }
                rm.addProcess(Process(pId));
            }
            else if (type == 'R')
            { // Add Resource
                int rId, count;
                if (!(ss >> rId >> count))
                {
                    send_error("Invalid Resource definition");
                    continue;
                }
                rm.addResource(Resource(rId, count));
            }
            else if (type == 'M')
            { // Declare Max Need
                int pId, rId, count;
                if (!(ss >> pId >> rId >> count))
                {
                    send_error("Invalid Max Need definition");
                    continue;
                }
                rm.declareMaxResources(pId, rId, count);
            }
            else if (type == 'E')
            { // Execute Event
                int pId, rId, count;
                string action;
                if (!(ss >> pId >> action >> rId >> count))
                {
                    send_error("Invalid Event definition");
                    continue;
                }
                if (action == "REQUEST")
                    rm.requestResource(pId, rId, count);
                else if (action == "RELEASE")
                    rm.releaseResource(pId, rId, count);
            }
            else if (type == 'X')
            {   // 'X' for eXamine (just send state)
                // Do nothing, state is sent below.
            }
            else if (type == 'C')
            { // 'C' for reCovery
                if (rm.strategy == DeadlockStrategy::DETECT)
                {
                    rm.recoveryAgent.initiateRecovery(rm);
                }
                else
                {
                    rm.log("Recovery only available in DETECT mode.");
                }
            }
            else
            {
                send_error("Unknown command type: " + string(1, type));
            }

            // After EVERY command, send the complete system state back to Python.
            printStateAsJson(rm);
        }
        catch (const exception &e)
        {
            send_error("C++ Exception: " + string(e.what()));
        }
        catch (...)
        {
            send_error("Unknown C++ exception.");
        }
    }
    return 0;
}