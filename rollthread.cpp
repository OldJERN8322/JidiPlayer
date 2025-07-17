#include "rollqueue.h"
#include "rolling.h"
#include <chrono>
#include <thread>
#include <atomic>

extern std::atomic<double> midiPlayheadSeconds;
extern std::atomic<bool> playing;
extern std::atomic<bool> finish;
extern std::atomic<bool> midiPaused;

struct PrebufferedNote {
    double time;
    int pitch;
    int track;
};

std::mutex prebufferMutex;
std::deque<PrebufferedNote> notePrebuffer;
std::atomic<bool> bufferThreadRunning{ false };

const double earlyWindow = 0.6; // seconds ahead of time to spawn early notes

void QueuePrebufferedNote(double time, int pitch, int track) {
    std::lock_guard<std::mutex> lock(prebufferMutex);
    notePrebuffer.push_back({ time, pitch, track });
}

void StartRollThread() {
    if (bufferThreadRunning) return;
    bufferThreadRunning = true;
    std::thread([] {
        while (bufferThreadRunning) {
            if (!playing || finish) {
                std::this_thread::sleep_for(std::chrono::milliseconds(10));
                continue;
            }

            double currentTime = midiPlayheadSeconds.load();
            earlyWindow;

            {
                std::lock_guard<std::mutex> lock(prebufferMutex);
                while (!notePrebuffer.empty()) {
                    const auto& n = notePrebuffer.front();
                    if (n.time <= currentTime + earlyWindow) {
                        AddRollNote(n.pitch, n.track, n.time - currentTime);
                        notePrebuffer.pop_front();
                    }
                    else {
                        break;
                    }
                }
            }

            std::this_thread::sleep_for(std::chrono::milliseconds(1));
        }
        }).detach();
}

void StopRollThread() {
    bufferThreadRunning = false;
}

void ProcessRollQueue(double currentTime) {
    std::lock_guard<std::mutex> lock(prebufferMutex);
    while (!notePrebuffer.empty()) {
        const auto& n = notePrebuffer.front();
        if (n.time <= currentTime + earlyWindow) {
            float timeUntilPlay = static_cast<float>(n.time - currentTime);
            AddRollNote(n.pitch, n.track, timeUntilPlay); // Negative means early
            notePrebuffer.pop_front();
        }
        else {
            break;
        }
    }
}