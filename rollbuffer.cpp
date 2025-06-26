// Shared buffer to offload note visuals separately from MIDI dispatch

#include "rollqueue.h"
#include <queue>
#include <mutex>
#include <utility>

// Used in playMidiAsync()
std::mutex rollQueueMutex;
std::queue<std::pair<int, int>> pendingRollQueue; // (pitch, track)

void AddRollNote(int pitch, int track) {
    AddRollNote(pitch, track, 0.05f);  // default fallback
}

void QueueRollNote(int pitch, int track) {
    std::lock_guard<std::mutex> lock(rollQueueMutex);
    if (pendingRollQueue.size() < 10000)
        pendingRollQueue.emplace(pitch, track);
}

// Replace AddRollNote() calls inside playMidiAsync() with:
//     QueueRollNote(data1, mev->track);

// Then in graphraylib.cpp each frame before drawing:
void ProcessRollQueue() {
    std::lock_guard<std::mutex> lock(rollQueueMutex);
    while (!pendingRollQueue.empty()) {
        auto [pitch, track] = pendingRollQueue.front();
        pendingRollQueue.pop();
        AddRollNote(pitch, track);
    }
}
