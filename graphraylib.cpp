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
#include <iostream>

extern std::atomic<double> midiPlayheadSeconds;
extern std::atomic<bool> playing;
extern std::atomic<bool> finish;

void playMidiAsync(const std::string& filename);
void DrawRollingNotes(float scrollSpeed, int screenHeight);

int graphrun(const std::string& filename) {
    const int screenWidth = 1280;
    const int screenHeight = 720;

    InitWindow(screenWidth, screenHeight, "Jidi Player (WIP)");
    SetTargetFPS(60);

    std::thread midiThread(playMidiAsync, filename);

    float scrollOffset = 0.0f;
    const float scrollSpeed = 1280.0f;

    while (!WindowShouldClose() || playing) {
        float frameTime = GetFrameTime();
        scrollOffset += scrollSpeed * frameTime;

        BeginDrawing();
        ClearBackground(BLACK);

        if (playing) {
            DrawRollingNotes(scrollSpeed, screenHeight);
            DrawText("Playing MIDI...", 10, 10, 30, GREEN);
        }
        else if (finish) {
            DrawText("MIDI Finished. Window will close shortly...", 10, 10, 30, ORANGE);
        }
        else {
            DrawText("MIDI Loading...", 10, 10, 30, WHITE);
        }

        DrawText(TextFormat("Scroll offset: %.2f", scrollOffset), 10, 50, 20, RAYWHITE);
        DrawFPS(10, 690);
        EndDrawing();
    }

    midiThread.join();
    std::this_thread::sleep_for(std::chrono::seconds(3));
    CloseWindow();
    return 0;
}
