#pragma once
#include <mutex>
#include <queue>

extern std::mutex rollQueueMutex;
extern std::queue<std::pair<int, int>> pendingRollQueue;

void QueueRollNote(int pitch, int track);
void ProcessRollQueue();
