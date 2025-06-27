#include "notecounter.h"

std::atomic<int> noteCounter = 0;
std::mutex noteCounterMutex;
std::unordered_set<int> activeNoteSet;
