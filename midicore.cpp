#include "MidiFile.h"
#include <OmniMIDI.h>
#include <atomic>
#include <chrono>
#include <iostream>
#include <thread>
#include <string>

using namespace smf;

std::atomic<double> midiPlayheadSeconds{ 0 };
std::atomic<bool> playing{ false };
std::atomic<bool> finish{ false };

extern "C" {
    BOOL KDMAPI(InitializeKDMAPIStream)();
    BOOL KDMAPI(TerminateKDMAPIStream)();
    VOID KDMAPI(SendDirectData)(DWORD dwMsg);
}

bool StartOmniMIDI() {
    BOOL ok = InitializeKDMAPIStream();
    if (!ok) {
        std::cerr << "OmniMIDI initialization failed!\n";
    }
    else {
        std::cout << "OmniMIDI KDMAPI initialized successfully.\n";
    }
    return ok;
}

void StopOmniMIDI() {
    std::cout << "Terminating KDMAPI stream.\n";
    TerminateKDMAPIStream();
}

void SendMIDIMessage(DWORD status, DWORD data1, DWORD data2) {
    DWORD message = status | (data1 << 8) | (data2 << 16);
    SendDirectData(message);
}

void playMidiAsync(const std::string& filename) {
    finish = false;
    if (!StartOmniMIDI()) {
        playing = false;
        return;
    }

    MidiFile midi;
    if (!midi.read(filename)) {
        std::cerr << "Failed to load MIDI file: " << filename << "\n";
        StopOmniMIDI();
        return;
    }

    midi.joinTracks();
    midi.doTimeAnalysis();

    playing = true;
    auto start = std::chrono::high_resolution_clock::now();

    for (int i = 0; i < midi[0].size() && playing; ++i) {
        MidiEvent& mev = midi[0][i];
        if (mev.isMeta()) continue;

        auto now = std::chrono::high_resolution_clock::now();
        double elapsed = std::chrono::duration<double>(now - start).count();
        midiPlayheadSeconds = elapsed;
        double waitTime = mev.seconds - elapsed;
        if (waitTime > 0) std::this_thread::sleep_for(std::chrono::duration<double>(waitTime));

        DWORD status = (mev[0] & 0xF0) | (mev.getChannel() & 0x0F);
        DWORD data1 = mev[1];
        DWORD data2 = mev[2];
        SendMIDIMessage(status, data1, data2);
    }

    finish = true;
    playing = false;
    StopOmniMIDI();
}