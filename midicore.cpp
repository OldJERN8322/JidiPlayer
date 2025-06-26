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
#include <map>
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

std::map<int, double> tempoMap; // tick -> tempoBPM
std::atomic<double> currentTempoBPM;

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

    // Collect tempo changes: tick -> tempo
    for (int i = 0; i < midi[0].getEventCount(); ++i) {
        const auto& e = midi[0][i];
        if (e.isTempo()) {
            tempoMap[e.tick] = e.getTempoBPM();
        }
    }
    if (tempoMap.empty()) tempoMap[0] = 180.0;

    // Build tick->time map
    std::map<int, double> tickToTime;
    double currentTime = 0.0;
    double currentTempo = tempoMap.begin()->second;
    int lastTick = 0;

    for (auto it = tempoMap.begin(); it != tempoMap.end(); ++it) {
        int t = it->first;
        double bpm = it->second;
        if (t > lastTick) {
            double segmentSeconds = ((double)(t - lastTick) * 60.0) / (currentTempo * tpq);
            currentTime += segmentSeconds;
            tickToTime[t] = currentTime;
        }
        else {
            tickToTime[t] = currentTime;
        }
        lastTick = t;
        currentTempo = bpm;
    }

    currentTempoBPM.store(tempoMap.begin()->second);
    playing = true;
    finish = false;

    std::deque<TimedEvent> events;
    for (int i = 0; i < midi[0].getEventCount(); ++i) {
        auto* mev = &midi[0][i];
        if (!mev->isMeta()) events.push_back({ mev->tick, mev });
    }

    auto start = std::chrono::high_resolution_clock::now();

    while (!events.empty() && playing) {
        auto now = std::chrono::high_resolution_clock::now();
        double elapsed = std::chrono::duration<double>(now - start).count();
        midiPlayheadSeconds = elapsed;

        int dispatched = 0;
        const int kMaxEventsPerFrame = 200;
        while (!events.empty()) {
            int evTick = events.front().tick;
            double evTime = 0.0;

            auto upper = tickToTime.upper_bound(evTick);
            if (upper == tickToTime.begin()) {
                evTime = 0.0;
            }
            else {
                auto prev = std::prev(upper);
                int prevTick = prev->first;
                double baseTime = prev->second;
                double bpm = tempoMap[prevTick];
                currentTempoBPM.store(bpm); // update tempo visual here
                evTime = baseTime + ((evTick - prevTick) * 60.0) / (bpm * tpq);
            }

            if (evTime > elapsed || dispatched >= kMaxEventsPerFrame) break;

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

        std::this_thread::sleep_for(std::chrono::milliseconds(1));
    }

    playing = false;
    finish = true;
    StopOmniMIDI();
}