#include "GameManager.h"
#include <cstdlib>
#include <ctime>
#include <iostream>

int main() {
    // Seed random number generator once at startup
    std::srand(static_cast<unsigned>(std::time(nullptr)));

    // Save file lives next to the executable / in the working directory
    const std::string saveFile = "pet_save.json";

    try {
        GameManager gm(saveFile);
        gm.run();
    } catch (const std::exception& ex) {
        std::cerr << "Fatal error: " << ex.what() << "\n";
        return 1;
    }
    return 0;
}
