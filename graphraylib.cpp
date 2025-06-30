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
#include "midipause.h"

extern std::atomic<double> midiPlayheadSeconds;
extern std::atomic<bool> playing;
extern std::atomic<bool> midiPaused;
extern std::atomic<bool> finish;
extern std::atomic<double> currentTempoBPM;
extern std::vector<int> activeNotes;

void playMidiAsync(const std::string& filename);
void DrawRollingNotes(float scrollSpeed, int screenHeight);

int graphrun(const std::string& filename) {
    const int screenWidth = 1280;
    const int screenHeight = 720;

    std::string basename = filename.substr(filename.find_last_of("/\\") + 1);
    InitWindow(screenWidth, screenHeight, TextFormat("Jidi Player - %s", basename.c_str()));
    //SetTargetFPS(60); //Capped

    std::thread midiThread(playMidiAsync, filename);

    float scrollOffset = 0.0f;
    const float scrollSpeed = 1920.0f;

    double lastFrameTime = GetTime();
    
    while (!WindowShouldClose() || playing) {
        double now = GetTime();
        float frameTime = static_cast<float>(now - lastFrameTime);  
        lastFrameTime = now;

        if (playing) scrollOffset += scrollSpeed * frameTime;
        ProcessRollQueue();

        if (IsKeyPressed(KEY_SPACE)) {
            HandleMidiPauseInput(true);
        }

        BeginDrawing();
        ClearBackground(BLACK);
        DrawRollingNotes(scrollSpeed, screenHeight);

        if (finish) {
            DrawText("MIDI Finished. You can close window.", 10, 10, 30, YELLOW);
        }
        else if (midiPaused) {
            DrawText("MIDI Paused - Press spacebar to continue.", 10, 10, 30, YELLOW);
            DrawText("[Paused]", 580, 345, 30, RED);
        }
        else if (playing) {
            DrawText("Playing MIDI...", 10, 10, 30, GREEN);
        }
        else {
            DrawText("MIDI Loading...", 10, 10, 30, WHITE);
        }

        DrawText(TextFormat("Note counter: %d", noteCounter.load()), 10, 50, 20, WHITE);
        DrawText(TextFormat("Time: %.2f s", midiPlayheadSeconds.load()), 10, 70, 20, WHITE);
        DrawText(TextFormat("BPM: %.1f", currentTempoBPM.load()), 10, 90, 20, WHITE);
        DrawText("Pre-Release v1.0.1 (Build: 26) - Uncapped", 10, 670, 20, YELLOW);
        DrawFPS(10, 690);
        EndDrawing();
    }

    midiThread.join();
    CloseWindow();
    return 0;
}