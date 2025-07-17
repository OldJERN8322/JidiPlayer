#include <deque>
#include <mutex>
#include <set>
#include <map>
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
constexpr float kLatencyOffset = 0.096f;
constexpr float kExtraPrebufferTime = 0.300f;
constexpr int kMaxNotes = 4096;
constexpr float kNoteHeight = 5.6f;

float pitchYCache[128];

void InitPitchYCache() {
    for (int i = 0; i < 128; ++i) {
        pitchYCache[i] = 720.0f - i * kNoteHeight;
    }
}

void AddRollNote(int pitch, int track, float duration = -1.0f) {
    std::lock_guard<std::mutex> lock(rollMutex);

    float lifetime = (duration >= 0.0f)
        ? duration
        : (kExtraPrebufferTime + kLatencyOffset + ((1280.0f - kScreenCenter) / kScrollSpeed));

    float startX = kScreenCenter + (kScrollSpeed * lifetime);

    // Prevent duplicate active pitch
    for (const auto& note : rollingNotes) {
        if (note.pitch == pitch && note.x > 640.0f && note.early) return;
    }

    rollingNotes.emplace_back(NoteRoll{ startX, pitch, track, 120.0f, lifetime, duration < 0.0f });
    heldKeys.insert(pitch);
}

void RemoveHeldKey(int pitch) {
    std::lock_guard<std::mutex> lock(rollMutex);
    heldKeys.erase(pitch);
}

void DrawRollingNotes(float scrollSpeed, int screenCenterX) {
    std::lock_guard<std::mutex> lock(rollMutex);
    float frameDelta = scrollSpeed * GetFrameTime();

    if (rollingNotes.size() > kMaxNotes) {
        rollingNotes.erase(rollingNotes.begin(), rollingNotes.begin() + 1024);
    }

    for (auto& note : rollingNotes) {
        note.x -= frameDelta;
    }

    for (const auto& note : rollingNotes) {
        if (note.x + note.width < -640.0f || note.x > 1920.0f) continue;

        float y = pitchYCache[note.pitch];

        static const Color palette[8] = { RED, ORANGE, YELLOW, LIME, SKYBLUE, PURPLE, PINK, BROWN };
        Color base = palette[note.track % 8];

        Color color = base;
        float drawWidth = scrollSpeed * note.lifetime;
        Vector2 pos = { note.x - (float)screenCenterX, y };
        Vector2 size = { drawWidth, kNoteHeight };

        DrawRectangleV(pos, size, color);
    }
}
