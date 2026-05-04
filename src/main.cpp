#include "TuiApp.h"
#include <cstdlib>
#include <ctime>
#include <iostream>

int main() {
    std::srand(static_cast<unsigned>(std::time(nullptr)));

    const std::string saveFile       = "savegame_v2.json";
    const std::string legacySaveFile = "pet_save.json";

    try {
        TuiApp app(saveFile, legacySaveFile);
        app.run();
    } catch (const std::exception& ex) {
        std::cerr << "Fatal error: " << ex.what() << "\n";
        return 1;
    }
    return 0;
}
