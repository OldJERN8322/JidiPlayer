#pragma once
#include <mutex>
#include <queue>

// Queued visual notes to render from playMidiAsync
extern std::mutex rollQueueMutex;
extern std::queue<std::pair<int, int>> pendingRollQueue;

void QueueRollNote(int pitch, int track);
void ProcessRollQueue();

// Visual note renderer (must be defined in rolling.cpp)
void AddRollNote(int pitch, int track);

// IMPORTANT: call ProcessRollQueue() from graphraylib.cpp every frame BEFORE DrawRollingNotes()