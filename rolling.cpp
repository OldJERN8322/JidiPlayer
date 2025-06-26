#include "raylib.h"
#include <vector>
#include <mutex>
#include <algorithm>
#include <atomic>
#include <chrono>

extern std::vector<int> activeNotes;
extern std::vector<int> activeTracks;
extern std::mutex noteMutex;

struct NoteRoll {
    float x;
    int pitch;
    int track;
    float width;
};

std::mutex rollMutex;
std::vector<NoteRoll> rollingNotes;

void AddRollNote(int pitch, int track) {
    std::lock_guard<std::mutex> lock(rollMutex);

    // Don't add if there's already a note of same pitch near entry zone
    for (const auto& note : rollingNotes) {
        if (note.pitch == pitch && note.x > 1250.0f) return;
    }

    if (rollingNotes.size() < 3000)
        rollingNotes.emplace_back(NoteRoll{ 1280.0f, pitch, track, 100.0f });
}

void DrawRollingNotes(float scrollSpeed, int screenHeight) {
    const int screenCenterX = 640;

    static double lastTime = GetTime();
    double now = GetTime();
    double delta = now - lastTime;
    lastTime = now;

    if (delta > 0.5) delta = 0.016; // freeze protection

    std::lock_guard<std::mutex> lock(rollMutex);
    std::lock_guard<std::mutex> activeLock(noteMutex);

    float moveStep = scrollSpeed * static_cast<float>(delta);
    const size_t maxDraw = 1000;
    size_t drawn = 0;

    auto pitchToY = [&](int pitch) {
        return 20 + (127 - pitch) * (screenHeight - 40) / 128.0f;
        };

    for (auto& note : rollingNotes) {
        note.x -= moveStep;
    }

    for (const auto& note : rollingNotes) {
        if (note.x + note.width < 0 || note.x > 1280) continue;
        if (++drawn > maxDraw) break;

        float y = pitchToY(note.pitch);

        Color color;
        switch (note.track % 6) {
        case 0: color = RED; break;
        case 1: color = GREEN; break;
        case 2: color = BLUE; break;
        case 3: color = ORANGE; break;
        case 4: color = PURPLE; break;
        case 5: color = YELLOW; break;
        default: color = GRAY; break;
        }

        DrawRectangle(static_cast<int>(note.x - screenCenterX + 640), static_cast<int>(y), static_cast<int>(note.width), 8, Fade(color, 0.6f));
    }

    for (size_t i = 0; i < activeNotes.size(); ++i) {
        int pitch = activeNotes[i];
        int track = (i < activeTracks.size()) ? activeTracks[i] : 0;
        float y = pitchToY(pitch);

        Color color;
        switch (track % 6) {
        case 0: color = RED; break;
        case 1: color = GREEN; break;
        case 2: color = BLUE; break;
        case 3: color = ORANGE; break;
        case 4: color = PURPLE; break;
        case 5: color = YELLOW; break;
        default: color = WHITE; break;
        }

        DrawRectangle(640, static_cast<int>(y), 10, 8, color);
    }

    // Prune offscreen
    if (rollingNotes.size() > 4000) {
        rollingNotes.erase(
            rollingNotes.begin(),
            rollingNotes.begin() + (rollingNotes.size() - 3000)
        );
    }

    rollingNotes.erase(
        std::remove_if(rollingNotes.begin(), rollingNotes.end(), [](const NoteRoll& n) {
            return n.x + n.width < 0;
            }),
        rollingNotes.end()
    );
}
