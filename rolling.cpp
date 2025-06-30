#include "rolling.h"
#include "rollqueue.h"
#include "rollthread.h"
#include <deque>
#include <mutex>
#include <set>
#include <raylib.h>

struct NoteRoll {
    float x;
    int pitch;
    int track;
    float width;
    float lifetime;
    bool early;
};

std::deque<NoteRoll> rollingNotes;
std::mutex rollMutex;
std::set<int> heldKeys;

constexpr float kScrollSpeed = 1920.0f;
constexpr float kScreenCenter = 640.0f;

float pitchToY(int pitch) {
    return 720.0f - (pitch * 5.6f);
}

void AddRollNote(int pitch, int track, float duration) {
    std::lock_guard<std::mutex> lock(rollMutex);
    for (const auto& note : rollingNotes) {
        if (note.pitch == pitch && note.x > 1200.0f) return;
    }

    float startX = 1280.0f;
    float lifetime = (duration >= 0.0f) ? duration : (startX - kScreenCenter) / kScrollSpeed;
    bool isEarly = (duration < 0.0f);
    rollingNotes.emplace_back(NoteRoll{ startX, pitch, track, 120.0f, lifetime, isEarly });
}

void DrawRollingNotes(float scrollSpeed, int screenCenterX) {
    std::lock_guard<std::mutex> lock(rollMutex);
    float frameDelta = scrollSpeed * GetFrameTime();

    if (rollingNotes.size() > 4096) {
        rollingNotes.erase(rollingNotes.begin(), rollingNotes.begin() + 960);
    }

    for (auto& note : rollingNotes) {
        note.x -= frameDelta;
        if (note.early && note.x <= screenCenterX && note.lifetime > 0.0f) {
            note.early = false;
        }
    }

    for (const auto& note : rollingNotes) {
        if (note.x + note.width < 0 || note.x > 1280) continue;

        float y = pitchToY(note.pitch);

        static const Color palette[8] = { RED, ORANGE, YELLOW, LIME, SKYBLUE, PURPLE, PINK, BROWN };
        Color base = palette[note.track % 8];

        Color color = base;

        float drawWidth = scrollSpeed * note.lifetime;
        Vector2 pos = { note.x - (float)screenCenterX, y };
        Vector2 size = { drawWidth, 5.6f };

        DrawRectangleV(pos, size, color);
    }
}
