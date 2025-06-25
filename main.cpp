#include "MidiFile.h"
#include "Options.h"

#include <iostream>
#include <string>
#include <thread>
#include <chrono>
#include <map>

extern int graphrun(); // Function to run the graphics with raylib, defined in graphics.cpp

#include <OmniMIDI.h>

extern "C" {
    BOOL KDMAPI(InitializeKDMAPIStream)();
    BOOL KDMAPI(TerminateKDMAPIStream)();
    VOID KDMAPI(SendDirectData)(DWORD dwMsg);
}

bool StartOmniMIDI()
{
    if (!InitializeKDMAPIStream()) {
        std::cerr << "Failed to initialize OmniMIDI stream." << std::endl;
        return false;
    }
    return true;
}

void StopOmniMIDI()
{
    TerminateKDMAPIStream();
}

void SendMIDIMessage(DWORD status, DWORD data1, DWORD data2)
{
    DWORD message = status | (data1 << 8) | (data2 << 16);
    SendDirectData(message);
}

using namespace smf;

void playMidi(const std::string& filename) {
    StartOmniMIDI();

    MidiFile midi;
    if (!midi.read(filename)) {
        std::cerr << "Failed to load MIDI file." << std::endl;
        StopOmniMIDI();
        return;
    }

    int trackCount = midi.getTrackCount();
    std::cout << "Loaded MIDI file with " << trackCount << " tracks." << std::endl;

    for (int i = 0; i < trackCount; ++i) {
        std::cout << "Track " << i + 1 << " has " << midi[i].size() << " events." << std::endl;
    }

    std::map<int, int> instruments; // channel -> program number
    for (int i = 0; i < trackCount; ++i) {
        for (int j = 0; j < midi[i].size(); ++j) {
            MidiEvent& ev = midi[i][j];
            if ((ev[0] & 0xF0) == 0xC0) { // Program Change
                int channel = ev.getChannel();
                int program = ev[1];
                instruments[channel] = program;
            }
        }
    }

    std::cout << "Instruments used by channel:" << std::endl;
    for (const auto& [channel, program] : instruments) {
        std::cout << "  Channel " << channel << ": Program " << program << std::endl;
    }

    midi.joinTracks();
    midi.doTimeAnalysis();

    std::cout << "Total events: " << midi[0].size() << std::endl;

    auto start = std::chrono::high_resolution_clock::now();

	std::cout << "Playing..." << std::endl;

    for (int i = 0; i < midi[0].size(); ++i) {
        MidiEvent& mev = midi[0][i];
        if (mev.isMeta()) continue;

        auto now = std::chrono::high_resolution_clock::now();
        double elapsed = std::chrono::duration<double>(now - start).count();
        double waitTime = mev.seconds - elapsed;
        if (waitTime > 0) {
            std::this_thread::sleep_for(std::chrono::duration<double>(waitTime));
        }

        if (mev.isNoteOn() || mev.isNoteOff() || mev.isController()) {
            DWORD status = (mev[0] & 0xF0) | (mev.getChannel() & 0x0F);
            DWORD data1 = mev[1];
            DWORD data2 = mev[2];
            SendMIDIMessage(status, data1, data2);
        }
        else if ((mev[0] & 0xF0) == 0xE0) {
            DWORD channel = mev.getChannel() & 0x0F;
            DWORD lsb = mev[1];
            DWORD msb = mev[2];
            DWORD status = 0xE0 | channel;
            DWORD midiMsg = status | (lsb << 8) | (msb << 16);
            SendDirectData(midiMsg);
        }
        else if ((mev[0] & 0xF0) == 0xC0) {
            DWORD status = 0xC0 | (mev.getChannel() & 0x0F);
            DWORD data1 = mev[1];
            DWORD midiMsg = status | (data1 << 8);
            SendDirectData(midiMsg);
        }
    }

	std::cout << "Finished. Closing in 5 seconds..." << std::endl;
    std::this_thread::sleep_for(std::chrono::seconds(5));

    StopOmniMIDI();
}

int main(int argc, char* argv[]) {
    std::string file;
    if (argc > 1) {
        file = argv[1];
    }
    else {
        std::cerr << "Usage: " << argv[0] << " <midi_file.mid>" << std::endl;
        return 1;
    }

    std::cout << "File loaded: " << file << std::endl << "Please wait..." << std::endl;
    playMidi(file);
    return 0;
    //return graphrun(); //Run the graphics with raylib.
}
