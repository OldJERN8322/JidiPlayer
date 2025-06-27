#pragma once
#include <atomic>

extern std::atomic<bool> midiPaused;

void HandleMidiPauseInput(bool spacePressed);