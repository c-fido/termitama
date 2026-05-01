#include "GameManager.h"
#include "Display.h"
#include "Minigame.h"
#include <chrono>
#include <fstream>
#include <iostream>
#include <limits>
#include <sstream>
#include <string>

// ── Constructor ───────────────────────────────────────────────────────────────

GameManager::GameManager(const std::string& saveFilePath)
    : pet_("Unnamed"), saveFilePath_(saveFilePath)
{
    // pet_ is replaced by loadGame() if a save file exists
}

// ── run ───────────────────────────────────────────────────────────────────────

void GameManager::run() {
    Display::clearScreen();
    Display::printBanner("  VIRTUAL PET  ");

    // Try to restore a saved pet
    if (!loadGame()) {
        std::cout << Color::BRIGHT_YELLOW
                  << "\nNo save file found. Starting a new game!\n"
                  << Color::RESET
                  << "\nWhat would you like to name your pet? ";
        std::string name;
        std::getline(std::cin, name);
        if (name.empty()) name = "Pixel";
        pet_ = Pet(name);
        std::cout << Color::BRIGHT_GREEN << "\nWelcome, " << name << "!\n" << Color::RESET;
    }

    while (running_) {
        if (!pet_.isAlive()) {
            Display::clearScreen();
            std::cout << Color::BRIGHT_RED << Color::BOLD
                      << "\n  " << pet_.getName() << " has passed away...\n\n"
                      << Color::RESET
                      << "  You had " << pet_.getInteractionCount()
                      << " interactions together.\n"
                      << "  Stage reached: " << pet_.getStageName() << "\n\n"
                      << Color::DIM << "  Thanks for playing.\n\n" << Color::RESET;
            break;
        }

        Display::clearScreen();
        printStatus();
        printMenu();

        std::cout << Color::BOLD << "\n  Your choice: " << Color::RESET;
        std::string input;
        std::getline(std::cin, input);

        if      (input == "1") handleFeed();
        else if (input == "2") handlePlay();
        else if (input == "3") handleSleep();
        else if (input == "4") { printStatus(); std::cout << "\nPress Enter to continue..."; std::cin.ignore(); }
        else if (input == "5") handleQuit();
        else {
            std::cout << Color::YELLOW << "  Invalid choice.\n" << Color::RESET;
        }
    }
}

// ── loadGame ─────────────────────────────────────────────────────────────────

bool GameManager::loadGame() {
    std::ifstream file(saveFilePath_);
    if (!file.is_open()) return false;

    std::ostringstream buf;
    buf << file.rdbuf();
    std::string json = buf.str();
    if (json.empty()) return false;

    try {
        pet_ = Pet::deserialize(json);
    } catch (const std::exception& ex) {
        std::cerr << Color::BRIGHT_RED
                  << "  Save file corrupted: " << ex.what()
                  << "\n  Starting fresh.\n" << Color::RESET;
        return false;
    }

    // Calculate how long the game was closed and apply retroactive stat decay.
    long long mins = minutesSinceLastSave();
    if (mins > 0) {
        std::cout << Color::BRIGHT_YELLOW
                  << "\nWelcome back! Checking on " << pet_.getName() << "...\n"
                  << Color::RESET;
        std::string report = pet_.applyOfflineDecay(mins);
        std::cout << report;
        std::cout << "\nPress Enter to continue...";
        std::cin.ignore(std::numeric_limits<std::streamsize>::max(), '\n');
    } else {
        std::cout << Color::BRIGHT_GREEN
                  << "\nWelcome back, " << pet_.getName() << "!\n" << Color::RESET;
    }
    return true;
}

// ── saveGame ──────────────────────────────────────────────────────────────────

void GameManager::saveGame() {
    pet_.stampSaveTime(); // record current wall-clock time before writing
    std::ofstream file(saveFilePath_);
    if (!file.is_open()) {
        std::cerr << Color::BRIGHT_RED
                  << "  Warning: could not write save file at " << saveFilePath_ << "\n"
                  << Color::RESET;
        return;
    }
    file << pet_.serialize();
    std::cout << Color::DIM << "\n  Game saved.\n" << Color::RESET;
}

// ── minutesSinceLastSave ──────────────────────────────────────────────────────
// We convert the POSIX timestamp stored in the Pet to a std::chrono time_point,
// then diff against now(). This gives us wall-clock elapsed time in minutes.
long long GameManager::minutesSinceLastSave() const {
    using namespace std::chrono;

    // Reconstruct a system_clock time_point from the stored POSIX timestamp
    auto lastSavePoint = system_clock::from_time_t(pet_.getLastSaved());
    auto now           = system_clock::now();
    auto elapsed       = duration_cast<minutes>(now - lastSavePoint);
    return elapsed.count();
}

// ── printStatus ───────────────────────────────────────────────────────────────

void GameManager::printStatus() const {
    Display::printBanner("  " + pet_.getName() + "  [" + pet_.getStageName() + "]  ");

    // ASCII art
    std::cout << "\n" << Display::getAsciiArt(pet_.getStage(), pet_.getCurrentMood()) << "\n";

    // Stats
    const auto& s = pet_.getStats();
    std::cout << Display::statLine("Hunger",    s.hunger)    << "\n"
              << Display::statLine("Happiness", s.happiness) << "\n"
              << Display::statLine("Energy",    s.energy)    << "\n"
              << Display::statLine("Health",    s.health)    << "\n";

    // Sick indicator
    if (pet_.isSick())
        std::cout << Color::BRIGHT_RED << Color::BOLD << "\n  !! " << pet_.getName()
                  << " is SICK !!\n" << Color::RESET;

    // Evolution progress
    int next = pet_.nextEvolutionAt();
    if (next > 0) {
        int count = pet_.getInteractionCount();
        std::cout << Color::DIM << "\n  Evolution progress: " << count << "/" << next
                  << " interactions\n" << Color::RESET;
    } else {
        std::cout << Color::BRIGHT_MAGENTA << "\n  " << pet_.getName()
                  << " has reached full evolution!\n" << Color::RESET;
    }

    Display::printDivider();
}

// ── printMenu ────────────────────────────────────────────────────────────────

void GameManager::printMenu() const {
    std::cout << Color::BOLD
              << "\n  1) Feed    2) Play    3) Sleep\n"
              << "  4) Status  5) Quit\n"
              << Color::RESET;
}

// ── handleFeed ────────────────────────────────────────────────────────────────

void GameManager::handleFeed() {
    std::cout << Color::BRIGHT_GREEN << "\n  Nom nom nom! " << pet_.getName()
              << " is eating...\n" << Color::RESET;
    pet_.feed(30);
    std::string event = postActionTick();
    if (!event.empty())
        std::cout << "\n" << event << "\n";
    std::cout << "\nPress Enter to continue...";
    std::cin.ignore(std::numeric_limits<std::streamsize>::max(), '\n');
}

// ── handlePlay ───────────────────────────────────────────────────────────────

void GameManager::handlePlay() {
    std::cout << Color::BRIGHT_YELLOW << "\n  Time to play!\n" << Color::RESET;
    int bonus = Minigame::runGuessingGame();
    pet_.receiveHappinessBoost(bonus);
    std::string event = postActionTick();
    if (!event.empty())
        std::cout << "\n" << event << "\n";
    std::cout << "\nPress Enter to continue...";
    std::cin.ignore(std::numeric_limits<std::streamsize>::max(), '\n');
}

// ── handleSleep ──────────────────────────────────────────────────────────────

void GameManager::handleSleep() {
    std::cout << Color::BLUE << "\n  " << pet_.getName()
              << " is resting...\n  z Z z Z\n" << Color::RESET;
    pet_.rest();
    std::string event = postActionTick();
    if (!event.empty())
        std::cout << "\n" << event << "\n";
    std::cout << "\nPress Enter to continue...";
    std::cin.ignore(std::numeric_limits<std::streamsize>::max(), '\n');
}

// ── handleQuit ───────────────────────────────────────────────────────────────

void GameManager::handleQuit() {
    saveGame();
    std::cout << Color::BRIGHT_CYAN << "  Goodbye! See you soon!\n" << Color::RESET;
    running_ = false;
}

// ── postActionTick ────────────────────────────────────────────────────────────
// Called after every player action:
//   1. Apply a small passive decay (5 in-game minutes per interaction).
//   2. Increment the interaction counter.
//   3. Check whether the pet should evolve.
//   4. Roll for a random event.
//   5. Auto-save.
std::string GameManager::postActionTick() {
    pet_.applyInteractionDecay();
    pet_.incrementInteractions();

    std::string output;

    bool evolved = pet_.checkAndEvolve();
    if (evolved) {
        output += Color::BRIGHT_MAGENTA + std::string(Color::BOLD) +
                  "\n  *** " + pet_.getName() + " EVOLVED into a " +
                  pet_.getStageName() + "! ***\n" + Color::RESET;
        output += Display::getAsciiArt(pet_.getStage(), pet_.getCurrentMood());
    }

    std::string event = pet_.triggerRandomEvent();
    if (!event.empty())
        output += "\n" + event;

    saveGame();
    return output;
}
