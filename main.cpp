#include <string>
#include <raylib.h>
#include <tinyfiledialogs.h>

int graphrun(const std::string& filename); // from graphraylib.cpp

int main() {
    const char* filters[] = { "*.mid", "*.midi" };
    const char* path = tinyfd_openFileDialog(
        "Select MIDI File", "", 2, filters, "MIDI files", 0);

    if (!path) {
        TraceLog(LOG_WARNING, "No file selected. Exiting.");
        return 0;
    }

    return graphrun(std::string(path));
}
