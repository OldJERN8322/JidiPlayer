#include "MidiFile.h"
#include <OmniMIDI.h>
#include <atomic>
#include <chrono>
#include <iostream>
#include <thread>
#include <string>
#include <vector>
#include <mutex>
#include <algorithm>
#include <deque>
#define DEBUG_PLAYBACK 1

using namespace smf;

std::atomic<double> midiPlayheadSeconds{ 0 };
std::atomic<bool> playing{ false };
std::atomic<bool> finish{ false };

std::mutex noteMutex;
std::vector<int> activeNotes;
std::vector<int> activeTracks;

void AddRollNote(int pitch, int track);

extern "C" {
    BOOL KDMAPI(InitializeKDMAPIStream)();
    BOOL KDMAPI(TerminateKDMAPIStream)();
    VOID KDMAPI(SendDirectData)(DWORD dwMsg);
}

bool StartOmniMIDI() {
    return InitializeKDMAPIStream();
}

void StopOmniMIDI() {
    TerminateKDMAPIStream();
}

void SendMIDIMessage(DWORD status, DWORD data1, DWORD data2) {
    DWORD message = status | (data1 << 8) | (data2 << 16);
    SendDirectData(message);
}

struct TimedEvent {
    int tick;
    smf::MidiEvent* ev;
};

void playMidiAsync(const std::string& filename) {
    if (!StartOmniMIDI()) {
        playing = false;
        finish = true;
        return;
    }

    smf::MidiFile midi;
    if (!midi.read(filename)) {
        std::cerr << "Failed to load MIDI file: " << filename << "\n";
        StopOmniMIDI();
        playing = false;
        finish = true;
        return;
    }

    midi.joinTracks();
    midi.doTimeAnalysis();

    int tpq = midi.getTicksPerQuarterNote();
    double tempo = 120.0; // fallback
    double tickDuration = 0.5 / tpq; // 120 BPM default = 0.5s/quarter = 500ms/quarter

    for (int i = 0; i < midi[0].getEventCount(); ++i) {
        const auto& e = midi[0][i];
        if (e.isTempo()) {
            tempo = e.getTempoBPM();
            tickDuration = 60.0 / (tempo * tpq);
        }
    }

    playing = true;
    finish = false;

    std::deque<TimedEvent> events;
    for (int i = 0; i < midi[0].getEventCount(); ++i) {
        auto* mev = &midi[0][i];
        if (!mev->isMeta()) {
            events.push_back({ mev->tick, mev });
        }
    }

    auto start = std::chrono::high_resolution_clock::now();
    const int kMaxEventsPerFrame = 200;

    while (!events.empty() && playing) {
        auto now = std::chrono::high_resolution_clock::now();
        double elapsed = std::chrono::duration<double>(now - start).count();
        double currentTick = elapsed / tickDuration;
        midiPlayheadSeconds = elapsed;

#if DEBUG_PLAYBACK
        if ((int)currentTick % 240 == 0) std::cout << "Tick: " << currentTick << ", Time: " << elapsed << "s\n";
#endif
        int dispatched = 0;
        int batch = 0;
        while (!events.empty() && events.front().tick <= currentTick && dispatched < kMaxEventsPerFrame) {
            smf::MidiEvent* mev = events.front().ev;
            events.pop_front();

            DWORD status = (mev->at(0) & 0xF0) | (mev->getChannel() & 0x0F);
            DWORD data1 = (*mev)[1];
            DWORD data2 = (*mev)[2];
            SendMIDIMessage(status, data1, data2);
            ++dispatched;

            if ((mev->at(0) & 0xF0) == 0x90 && data2 > 0) {
                std::lock_guard<std::mutex> lock(noteMutex);
                activeNotes.push_back(data1);
                activeTracks.push_back(mev->track);
                AddRollNote(data1, mev->track);
            }
            else if ((mev->at(0) & 0xF0) == 0x80 || ((mev->at(0) & 0xF0) == 0x90 && data2 == 0)) {
                std::lock_guard<std::mutex> lock(noteMutex);
                for (size_t j = 0; j < activeNotes.size(); ++j) {
                    if (activeNotes[j] == data1) {
                        activeNotes.erase(activeNotes.begin() + j);
                        activeTracks.erase(activeTracks.begin() + j);
                        break;
                    }
                }
            }
        }

#if DEBUG_PLAYBACK
        if (batch > 0) std::cout << "Dispatched: " << batch << " notes\n";
#endif

        if (dispatched >= kMaxEventsPerFrame) {
            std::this_thread::sleep_for(std::chrono::milliseconds(2)); // yield CPU if overrun
        }
        else {
            std::this_thread::sleep_for(std::chrono::milliseconds(1));
        }
    }

    playing = false;
    finish = true;
    StopOmniMIDI();
}
