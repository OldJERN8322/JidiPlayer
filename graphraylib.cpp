#ifndef NOMINMAX
#define NOMINMAX
#endif

#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif

#define NOGDI
#define NOUSER

// Add frame rate safety fallback for Black MIDI density

#include "raylib.h"
#include <atomic>
#include <thread>
#include <chrono>
#include <string>

extern std::atomic<double> midiPlayheadSeconds;
extern std::atomic<bool> playing;
extern std::atomic<bool> finish;
extern std::atomic<double> currentTempoBPM;

void playMidiAsync(const std::string& filename);
void DrawRollingNotes(float scrollSpeed, int screenHeight);

int graphrun(const std::string& filename) {
    const int screenWidth = 1280;
    const int screenHeight = 720;

    InitWindow(screenWidth, screenHeight, "Jidi Player (WIP)");
    // Remove SetTargetFPS to run uncapped
    // Add manual frame pacing if needed
    SetTargetFPS(60);

    std::thread midiThread(playMidiAsync, filename);

    float scrollOffset = 0.0f;
    const float scrollSpeed = 1920.0f;

    double lastFrameTime = GetTime();

    while (!WindowShouldClose() || playing) {
        double now = GetTime();
        float frameTime = static_cast<float>(now - lastFrameTime);
        lastFrameTime = now;

        if (playing) scrollOffset += scrollSpeed * frameTime;

        BeginDrawing();
        ClearBackground(BLACK);

        if (playing) {
            DrawRollingNotes(scrollSpeed, screenHeight);
            DrawText("Playing MIDI...", 10, 10, 30, GREEN);
            DrawText(TextFormat("BPM: %.1f", currentTempoBPM.load()), 10, 50, 20, WHITE);
        }
        else if (finish) {
            DrawText("MIDI Finished. Window will close shortly...", 10, 10, 30, ORANGE);
        }
        else {
            DrawText("MIDI Loading...", 10, 10, 30, WHITE);
        }

        DrawFPS(10, 690);
        EndDrawing();

        // Optional CPU throttle if overloaded
        if (frameTime < 1.0 / 300.0) std::this_thread::sleep_for(std::chrono::milliseconds(1));
    }

    midiThread.join();
    std::this_thread::sleep_for(std::chrono::seconds(3));
    CloseWindow();
    return 0;
}
