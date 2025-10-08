# Deadlock Master: An Operating Systems Simulation

This project is a C++ simulation of an operating system's resource manager, designed to handle two of the most critical challenges in concurrent programming: **deadlock** and **starvation**. [cite_start]The system, named "The Arbitrator", implements a modular architecture to intelligently detect, resolve, and prevent these issues in a simulated multi-process environment[cite: 64, 79].

This project was developed as part of the Phase 2 Project-Based Learning at Graphic Era University.

---

## Key Features

* [cite_start]**Modular Architecture:** The system is built around a central `ResourceManager` that uses specialized modules for different tasks, promoting clean and reusable code[cite: 96, 97, 108].
* [cite_start]**Deadlock Detection:** Implements a graph-based wait-for algorithm to accurately detect circular wait conditions among processes[cite: 100].
* **Intelligent Deadlock Recovery:** Features a custom, cost-based victim selection algorithm to resolve deadlocks. [cite_start]Instead of choosing a random process, it intelligently selects the victim that will cause the least disruption to the system[cite: 82].
* [cite_start]**Starvation Prevention:** Includes a `StarvationGuardian` module that implements the "Aging" technique, ensuring that processes that wait for a long time have their priority increased to guarantee eventual execution[cite: 83].
* **Dynamic Simulation Engine:** The simulation is not hard-coded. It is driven by a `scenario.txt` file, allowing users to define and test any number of complex process and resource interaction scenarios.

## Technologies Used

* **Language:** C++ (Standard: C++17)
* **Build System:**  g++
* **Version Control:** Git & GitHub

## How to Compile and Run

This project can be compiled using g++ on a system with a C++17 compatible compiler (like MinGW-w64 on Windows).

#### 1. Compilation

Navigate to the root directory of the project in your terminal. You can compile using either the provided Makefile or a direct g++ command.

**Using the Makefile (Recommended):**
```bash
make
```

**Using g++ directly:**
```bash
g++ -std=c++17 -Wall -Iinclude -o DeadlockMaster main.cpp src/*.cpp
```

#### 2. Running a Simulation

To run the simulation, you first need to provide a scenario. Copy the content of one of the predefined scenarios into a file named `scenario.txt` in the root project directory.

Then, run the executable:
```bash
./DeadlockMaster
```
The program will read `scenario.txt`, execute the simulation, and print the step-by-step output to the console.

## Predefined Scenarios

This project comes with four scenarios to demonstrate the system's capabilities:

1.  **Simple 2-Process Deadlock:** A classic circular wait between two processes and two resources. Demonstrates the basic deadlock detection and recovery.
2.  **No Deadlock (Successful Completion):** A scenario with multiple processes and resources that completes successfully without any deadlocks. This proves the system can handle safe states.
3.  **Complex 4-Process "Long Chain" Deadlock:** A more advanced deadlock involving a circular dependency among four processes. This showcases the robustness of the detection algorithm.
4.  **Starvation and Aging:** A special scenario designed to show the starvation prevention mechanism in action. It demonstrates a low-priority process having its priority boosted after waiting for too long.
