#pragma once
#include <atomic>
#include <mutex>
#include <unordered_set>

extern std::atomic<int> noteCounter;
extern std::mutex noteCounterMutex;
extern std::unordered_set<int> activeNoteSet;

inline void IncrementNoteCounterOnce(int pitch) {
    std::lock_guard<std::mutex> lock(noteCounterMutex);
    if (activeNoteSet.insert(pitch).second) {
        noteCounter++;
    }
}

inline void ReleaseNoteCounter(int pitch) {
    std::lock_guard<std::mutex> lock(noteCounterMutex);
    activeNoteSet.erase(pitch);
}