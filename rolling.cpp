#include "raylib.h"
#include "rollqueue.h" //forget this line here
#include <deque>
#include <mutex>
#include <algorithm>
#include <atomic>
#include <chrono>
#include <set>

extern std::vector<int> activeNotes;
extern std::vector<int> activeTracks;
extern std::mutex noteMutex;

struct NoteRoll {
    float x;
    int pitch;
    int track;
    float width;
    float lifetime;
};

std::mutex rollMutex;
std::deque<NoteRoll> rollingNotes;
std::set<int> heldKeys;

void AddRollNote(int pitch, int track, float duration = 0.2f) {
    std::lock_guard<std::mutex> lock(rollMutex);
    for (const auto& note : rollingNotes) {
        if (note.pitch == pitch && note.x > 1200.0f) return; // avoid overlaps
    }
    rollingNotes.emplace_back(NoteRoll{ 1280.0f, pitch, track, 120.0f, duration });
    if (rollingNotes.size() > 32768) {
        rollingNotes.erase(rollingNotes.begin(), rollingNotes.begin() + 1000);
    }
}

void PollInputKeys() {
    std::lock_guard<std::mutex> lock(noteMutex);
    for (int i = 0; i < 128; ++i) {
        if (IsKeyDown(i)) {
            if (!heldKeys.count(i)) {
                activeNotes.push_back(i);
                activeTracks.push_back(0);
                AddRollNote(i, 0, 0.5f);
                heldKeys.insert(i);
            }
        }
        else {
            heldKeys.erase(i);
        }
    }
}

void DrawRollingNotes(float scrollSpeed, int screenHeight) {
    const int screenCenterX = 640;
    static double lastTime = GetTime();
    double now = GetTime();
    double delta = now - lastTime;
    lastTime = now;
    if (delta > 0.5) delta = 0.016;

    std::lock_guard<std::mutex> lock(rollMutex);
    std::lock_guard<std::mutex> activeLock(noteMutex);

    float moveStep = scrollSpeed * static_cast<float>(delta);

    auto pitchToY = [&](int pitch) {
        return (127 - pitch) * (screenHeight) / 128.0f;
        };

    for (auto& note : rollingNotes) note.x -= moveStep;

    for (const auto& note : rollingNotes) {
        if (note.x + note.width < 0 || note.x > 1280) continue;

        float y = pitchToY(note.pitch);

        Color base;
        switch (note.track % 6) {
        case 0: base = GREEN; break;
        case 1: base = BLUE; break;
        case 2: base = ORANGE; break;
        case 3: base = PURPLE; break;
        case 4: base = YELLOW; break;
        case 5: base = SKYBLUE; break;
        default: base = GRAY; break;
        }

        Color color = Fade(base, 1.0f);
        float drawWidth = scrollSpeed * note.lifetime;
        Vector2 pos = { note.x - screenCenterX + 640.0f, y };
        Vector2 size = { drawWidth, 5.6f };
        DrawRectangleV(pos, size, color);
    }

    for (size_t i = 0; i < activeNotes.size(); ++i) {
        int pitch = activeNotes[i];
        int track = (i < activeTracks.size()) ? activeTracks[i] : 0;
        float y = pitchToY(pitch);
        Vector2 pos = { 1272.0f, y };
        Vector2 size = { 10.0f, 5.6f };
        DrawRectangleV(pos, size, WHITE);
    }

    while (!rollingNotes.empty()) {
        const auto& n = rollingNotes.front();
        if ((n.x + scrollSpeed * n.lifetime) < -50.0f) {
            rollingNotes.pop_front();
        }
        else {
            break;
        }
    }

    DrawText(TextFormat("Rendered notes: %d", (int)rollingNotes.size()), 10, 650, 20, WHITE);
}
