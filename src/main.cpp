#include "TuiApp.h"
#include <cstdlib>
#include <ctime>
#include <iostream>
#include <string>
#include <sys/stat.h>

static std::string dataDir() {
    const char* home = std::getenv("HOME");
    if (!home) home = ".";
    std::string dir = std::string(home) + "/.local/share/termitama";
    mkdir(dir.c_str(), 0755);
    return dir;
}

int main() {
    std::srand(static_cast<unsigned>(std::time(nullptr)));

    const std::string base           = dataDir();
    const std::string saveFile       = base + "/savegame_v2.json";
    const std::string legacySaveFile = base + "/pet_save.json";

    try {
        TuiApp app(saveFile, legacySaveFile);
        app.run();
    } catch (const std::exception& ex) {
        std::cerr << "Fatal error: " << ex.what() << "\n";
        return 1;
    }
    return 0;
}
