#include <iostream>
#include <vector>
#include <queue>
#include <algorithm>
#include <thread>
#include <chrono>
#include <cstdlib>
#include <iomanip>

using namespace std;

// Structure to represent a plane with attributes similar to a process in OS
struct Plane {
    int id;
    int arrivalTime;
    int fuelLevel;
    int size;
    int landingTime;
    bool emergency;

    Plane(int i, int at, int fuel, int s, int lt, bool e = false)
        : id(i), arrivalTime(at), fuelLevel(fuel), size(s), landingTime(lt), emergency(e) {}
};

// Scheduler class for Priority + SJF Scheduling
class Scheduler {
public:
    vector<Plane> readyQueue;

    static bool compare(const Plane &a, const Plane &b) {
        if (a.emergency != b.emergency) return a.emergency > b.emergency;
        if (a.fuelLevel != b.fuelLevel) return a.fuelLevel < b.fuelLevel;
        return a.landingTime < b.landingTime;
    }

    void addPlane(const Plane &p) {
        readyQueue.push_back(p);
    }

    Plane getNextPlane() {
        if (readyQueue.empty()) return Plane(-1, 0, 0, 0, 0);
        sort(readyQueue.begin(), readyQueue.end(), compare);
        Plane p = readyQueue.front();
        readyQueue.erase(readyQueue.begin());
        return p;
    }
};

// MemoryManager simulates airspace management with best-fit allocation
class MemoryManager {
    vector<int> memory;
public:
    MemoryManager(int size) : memory(size, 0) {}

    // Best-Fit Allocation
    int allocate(int size) {
        int bestStart = -1, bestSize = memory.size() + 1;
        for (int i = 0; i <= memory.size() - size; ++i) {
            int j = 0;
            while (j < size && memory[i + j] == 0) j++;
            if (j == size && size < bestSize) {
                bestStart = i;
                bestSize = size;
            }
        }
        if (bestStart != -1) {
            for (int i = bestStart; i < bestStart + size; ++i) memory[i] = 1;
        }
        return bestStart;
    }

    void deallocate(int start, int size) {
        for (int i = start; i < start + size; ++i) memory[i] = 0;
    }

    void display() {
        cout << "Airspace: ";
        for (int b : memory) cout << (b ? '#' : '.') << ' ';
        cout << endl;
    }

    // Banker’s Algorithm to check if state is safe before allocating
    bool isSafe(int reqSize) {
        int freeCount = 0;
        for (int m : memory) if (m == 0) freeCount++;
        return freeCount >= reqSize; // Safe only if enough contiguous or non-contiguous space
    }
};

// Table printing for plane data
void printPlaneTable(const vector<Plane>& planes) {
    cout << left << setw(6) << "ID" << setw(14) << "ArrivalTime" << setw(11) << "Fuel" << setw(13) << "Size(Mem)" << setw(14) << "LandingTime" << "Priority\n";
    cout << string(60, '-') << endl;
    for (const Plane& p : planes) {
        string prio = p.emergency ? "Emergency" : (p.fuelLevel <= 2 ? "High" : "Normal");
        cout << left << setw(6) << p.id << setw(14) << p.arrivalTime << setw(11) << p.fuelLevel << setw(13) << p.size << setw(14) << p.landingTime << prio << '\n';
    }
    cout << endl;
}

// Randomly trigger weather delay (visual only, doesn’t affect logic)
void simulateWeatherDelay() {
    if (rand() % 10 < 2) {
        cout << "Weather delay! ALL flights postponed.\n";
        this_thread::sleep_for(chrono::seconds(6));  // 6 seconds pause
    }
}

// Main simulation engine
void simulate(vector<Plane> incoming, string caseName) {
    Scheduler scheduler;
    MemoryManager memory(20);
    int time = 0;
    vector<pair<int, int>> gantt;

    cout << "\n================== Simulation: " << caseName << " ==================\n";
    printPlaneTable(incoming);

    while (!incoming.empty() || !scheduler.readyQueue.empty()) {
        simulateWeatherDelay();

        // Admit arriving planes
        auto it = incoming.begin();
        while (it != incoming.end()) {
            if (it->arrivalTime <= time) {
                scheduler.addPlane(*it);
                it = incoming.erase(it);
            } else {
                ++it;
            }
        }

        // Get next plane
        Plane current = scheduler.getNextPlane();
        if (current.id == -1) {
            cout << "Time " << time << ": No planes to schedule\n";
            time++;
            continue;
        }

        cout << "\nTime " << time << ": Scheduling Plane " << current.id << endl;

        // Deadlock Prevention via Banker’s Algorithm
        if (!memory.isSafe(current.size)) {
            cout << "Unsafe to allocate memory to Plane " << current.id << ". Potential deadlock! Delaying.\n";
            scheduler.addPlane(current);
            time++;
            continue;
        }

        // Best-Fit allocation
        int memIndex = memory.allocate(current.size);
        if (memIndex == -1) {
            cout << "No space in airspace for Plane " << current.id << ". Delayed.\n";
            scheduler.addPlane(current);
            time++;
            continue;
        }

        cout << "Plane " << current.id << " is landing.\n";
        gantt.push_back({current.id, current.landingTime});
        this_thread::sleep_for(chrono::milliseconds(200));
        time += current.landingTime;
        memory.deallocate(memIndex, current.size);
        memory.display();
    }

    // Gantt Chart
    cout << "\n================== Gantt Chart (Runway Usage) ==================\n";
    cout << left << setw(10) << "Step" << setw(12) << "Plane ID" << setw(16) << "Start Time" << "End Time\n";
    cout << string(50, '-') << endl;
    int t = 0;
    for (size_t i = 0; i < gantt.size(); ++i) {
        int planeID = gantt[i].first;
        int duration = gantt[i].second;
        cout << left << setw(10) << i + 1 << setw(12) << planeID << setw(16) << t << t + duration << '\n';
        t += duration;
    }
    cout << string(50, '=') << "\nSimulation complete.\n\n";
}

// Entry Point
int main() {
    // Input 1: Normal Traffic
    vector<Plane> normalPlanes = {
        Plane(1, 0, 5, 4, 3),
        Plane(2, 1, 4, 3, 2),
        Plane(3, 2, 6, 5, 4),
        Plane(4, 3, 1, 2, 1),
        Plane(5, 4, 2, 3, 3)
    };

    // Input 2: Emergency Case
    vector<Plane> emergencyPlanes = {
        Plane(1, 0, 5, 4, 3),
        Plane(2, 1, 4, 3, 2),
        Plane(6, 1, 1, 2, 1, true),
        Plane(3, 2, 6, 5, 4),
        Plane(5, 4, 1, 3, 3)
    };

    // Input 3: Deadlock Prevention Scenario
    vector<Plane> deadlockScenario = {
        Plane(7, 0, 3, 8, 3),
        Plane(8, 1, 2, 8, 2),
        Plane(9, 2, 1, 5, 2)
    };

    simulate(normalPlanes, "Normal Priority Scheduling");
    simulate(emergencyPlanes, "Emergency Case Scheduling");
    simulate(deadlockScenario, "Deadlock Prevention Scenario");

    return 0;
}
