#ifndef NOMINMAX
#define NOMINMAX
#endif

#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif

#define NOGDI
#define NOUSER

#include "raylib.h"
#include <string>
#include <atomic>
#include <thread>
#include <chrono>

extern std::atomic<double> midiPlayheadSeconds;
extern std::atomic<bool> playing;
extern std::atomic<bool> finish;

void playMidiAsync(const std::string& filename);

int graphrun(const std::string& filename) {
    const int screenWidth = 1280;
    const int screenHeight = 720;

    InitWindow(screenWidth, screenHeight, "Jidi Player (WIP)");
    SetTargetFPS(60);

    std::thread midiThread(playMidiAsync, filename);

    while (!WindowShouldClose() || playing) {
        BeginDrawing();
        ClearBackground(BLACK);

        if (playing) {
            DrawText("Playing MIDI...", 10, 10, 30, GREEN);
        }
        else if (finish) {
            DrawText("MIDI Finished. Window will close shortly...", 10, 10, 30, ORANGE);
        }
        else {
            DrawText("MIDI Loading...", 10, 10, 30, WHITE);
        }

        DrawFPS(10, 690);
        EndDrawing();
    }

    midiThread.join();
    std::this_thread::sleep_for(std::chrono::seconds(3));
    CloseWindow();
    return 0;
}
