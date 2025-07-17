#pragma once
#include <mutex>
#include <queue>

extern std::mutex rollQueueMutex;
extern std::queue<std::pair<int, int>> pendingRollQueue;

void AddRollNote(int pitch, int track, float duration);
void QueueRollNote(int pitch, int track);
void QueuePrebufferedNote(double time, int pitch, int track);
void ProcessRollQueue();
