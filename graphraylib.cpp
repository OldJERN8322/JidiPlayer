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
#include "rollqueue.h"
#include "notecounter.h"

extern std::atomic<double> midiPlayheadSeconds;
extern std::atomic<bool> playing;
extern std::atomic<bool> finish;
extern std::atomic<double> currentTempoBPM;
extern std::vector<int> activeNotes;

void playMidiAsync(const std::string& filename);
void DrawRollingNotes(float scrollSpeed, int screenHeight);

int graphrun(const std::string& filename) {
    const int screenWidth = 1280;
    const int screenHeight = 720;

    std::string basename = filename.substr(filename.find_last_of("/\\") + 1);
    InitWindow(screenWidth, screenHeight, TextFormat("Jidi Player (WIP) - %s", basename.c_str()));
    SetTargetFPS(60);

    std::thread midiThread(playMidiAsync, filename);

    float scrollOffset = 0.0f;
    const float scrollSpeed = 1920.0f; // MIDI speed

    double lastFrameTime = GetTime();

    while (!WindowShouldClose() || playing) {
        double now = GetTime();
        float frameTime = static_cast<float>(now - lastFrameTime);  
        lastFrameTime = now;

        if (playing) scrollOffset += scrollSpeed * frameTime;

        // Process queued MIDI visual events BEFORE rendering
        ProcessRollQueue();

        BeginDrawing();
        ClearBackground(BLACK);

        if (playing) {
            DrawRollingNotes(scrollSpeed, screenHeight);
            DrawText("Playing MIDI...", 10, 10, 30, GREEN);
            DrawText(TextFormat("Note counter: %d", noteCounter.load()), 10, 50, 20, WHITE);
            DrawText(TextFormat("Time: %.2f s", midiPlayheadSeconds.load()), 10, 70, 20, WHITE);
            DrawText(TextFormat("BPM: %.1f", currentTempoBPM.load()), 10, 90, 20, WHITE);
        }
        else if (finish) {
            DrawRollingNotes(scrollSpeed, screenHeight);
            DrawText("MIDI Finished. You can close window.", 10, 10, 30, YELLOW);
        }
        else {
            DrawText("MIDI Loading...", 10, 10, 30, WHITE);
        }

        DrawText("This crashpoint take slower midi...", 10, 630, 20, RED);
        DrawFPS(10, 690);
        EndDrawing();
    }

    midiThread.join();
    CloseWindow();
    return 0;
}