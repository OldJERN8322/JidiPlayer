#include "raylib.h"
#include <vector>
#include <mutex>
#include <algorithm>
#include <atomic>

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
    rollingNotes.push_back(NoteRoll{ 1280.0f, pitch, track, 160.0f });
}

void DrawRollingNotes(float scrollSpeed, int screenHeight) {
    const int screenCenterX = 640;

    std::lock_guard<std::mutex> lock(rollMutex);
    std::lock_guard<std::mutex> activeLock(noteMutex);

    for (auto& note : rollingNotes) {
        note.x -= scrollSpeed * GetFrameTime();
    }

    auto pitchToY = [&](int pitch) -> float {
        return 20 + (127 - pitch) * (screenHeight - 40) / 128.0f; // flip vertically
        };

    for (const auto& note : rollingNotes) {
        if (note.x + note.width < 0 || note.x > 1280) continue;
        float y = pitchToY(note.pitch);

        Color color = WHITE;
        switch (note.track % 6) {
        case 0: color = RED; break;
        case 1: color = GREEN; break;
        case 2: color = BLUE; break;
        case 3: color = ORANGE; break;
        case 4: color = PURPLE; break;
        case 5: color = YELLOW; break;
        }
        DrawRectangle(static_cast<int>(note.x - screenCenterX + 640), static_cast<int>(y), static_cast<int>(note.width), 8, Fade(color, 0.4f));
    }

    for (size_t i = 0; i < activeNotes.size(); ++i) {
        int pitch = activeNotes[i];
        int track = (i < activeTracks.size()) ? activeTracks[i] : 0;
        float y = pitchToY(pitch);

        Color color = WHITE;
        switch (track % 6) {
        case 0: color = RED; break;
        case 1: color = GREEN; break;
        case 2: color = BLUE; break;
        case 3: color = ORANGE; break;
        case 4: color = PURPLE; break;
        case 5: color = YELLOW; break;
        }
        DrawRectangle(640, static_cast<int>(y), 10, 8, color);
    }

    rollingNotes.erase(
        std::remove_if(rollingNotes.begin(), rollingNotes.end(), [](const NoteRoll& n) { return n.x + n.width < 0; }),
        rollingNotes.end()
    );
}
