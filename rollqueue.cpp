#include "rollqueue.h"
#include "rolling.h"
#include <mutex>
#include <queue>

// Actual definitions here (only defined once)
std::mutex rollQueueMutex;
std::queue<std::pair<int, int>> pendingRollQueue;

void QueueRollNote(int pitch, int track) {
    std::lock_guard<std::mutex> lock(rollQueueMutex);
    pendingRollQueue.emplace(pitch, track);
}

void ProcessRollQueue() {
    std::lock_guard<std::mutex> lock(rollQueueMutex);
    while (!pendingRollQueue.empty()) {
        auto [pitch, track] = pendingRollQueue.front();
        pendingRollQueue.pop();
        AddRollNote(pitch, track, -1.0f); // default duration will be calculated inside
    }
}
