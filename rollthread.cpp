#include "rollthread.h"
#include "rollqueue.h"
#include <queue>
#include <mutex>
#include <thread>
#include <atomic>
#include <chrono>

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
std::queue<PrebufferedNote> notePrebuffer;
std::atomic<bool> bufferThreadRunning{ false };

void QueuePrebufferedNote(double time, int pitch, int track) {
    std::lock_guard<std::mutex> lock(prebufferMutex);
    if (notePrebuffer.size() < 32768)
        notePrebuffer.push({ time, pitch, track });
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
            double earlyWindow = 0.3;

            {
                std::lock_guard<std::mutex> lock(prebufferMutex);
                while (!notePrebuffer.empty()) {
                    const auto& n = notePrebuffer.front();
                    if (n.time <= currentTime + earlyWindow) {
                        QueueRollNote(n.pitch, n.track);
                        notePrebuffer.pop();
                    }
                    else {
                        break;
                    }
                }
            }

            std::this_thread::sleep_for(std::chrono::milliseconds(2));
        }
        }).detach();
}

void StopRollThread() {
    bufferThreadRunning = false;
}
