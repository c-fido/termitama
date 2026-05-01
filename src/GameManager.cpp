#include "GameManager.h"
#include "Display.h"
#include "Minigame.h"
#include "Shop.h"
#include "Genetics.h"
#include <chrono>
#include <fstream>
#include <iostream>
#include <limits>
#include <sstream>
#include <string>
#include <algorithm>
#include <iomanip>

GameManager::GameManager(const std::string& saveFile,
                         const std::string& legacySaveFile)
    : saveFilePath_(saveFile), legacySavePath_(legacySaveFile)
{}

Pet& GameManager::activePet() {
    return *pets_[static_cast<size_t>(activePetIdx_)];
}
const Pet& GameManager::activePet() const {
    return *pets_[static_cast<size_t>(activePetIdx_)];
}

void GameManager::rebuildFSM() {
    fsm_ = std::make_unique<PetFSM>(activePet());
    fsm_->syncFromPet();
}

void GameManager::pause(const std::string& msg) {
    if (!msg.empty()) std::cout << msg;
    std::cout << "\nPress Enter to continue...";
    std::cin.ignore(std::numeric_limits<std::streamsize>::max(), '\n');
}

void GameManager::run() {
    Display::clearScreen();
    Display::printBanner("  VIRTUAL PET  v2.0  ");

    bool loaded = loadGame();
    if (!loaded) loaded = loadLegacyGame();

    if (!loaded) {
        std::cout << Color::BRIGHT_YELLOW
                  << "\nNo save file found. Starting a new game!\n" << Color::RESET
                  << "\nWhat would you like to name your first pet? ";
        std::string name;
        std::getline(std::cin, name);
        if (name.empty()) name = "Pixel";
        pets_.push_back(std::make_unique<Pet>(name));
        activePetIdx_ = 0;
        std::cout << Color::BRIGHT_GREEN << "\nWelcome, " << name << "!\n" << Color::RESET;
    }

    rebuildFSM();
    audio_.start();

    while (running_) {
        std::string fsmMsg = fsm_->tick();

        if (!activePet().isAlive()) {
            Display::clearScreen();
            std::cout << Color::BRIGHT_RED << Color::BOLD
                      << "\n  " << activePet().getName() << " has passed away...\n\n"
                      << Color::RESET
                      << "  Interactions: " << activePet().getInteractionCount() << "\n"
                      << "  Stage reached: " << activePet().getStageName() << "\n\n";

            if (pets_.size() > 1) {
                std::cout << Color::YELLOW
                          << "  You still have other pets. Visit the Pet Box.\n"
                          << Color::RESET;
                pause();
                for (int i = 0; i < static_cast<int>(pets_.size()); ++i) {
                    if (pets_[static_cast<size_t>(i)]->isAlive()) {
                        activePetIdx_ = i;
                        rebuildFSM();
                        break;
                    }
                }
                bool anyAlive = false;
                for (const auto& p : pets_)
                    if (p->isAlive()) { anyAlive = true; break; }
                if (!anyAlive) {
                    std::cout << Color::DIM << "  All pets have passed away. Thanks for playing.\n\n"
                              << Color::RESET;
                    break;
                }
                continue;
            }
            std::cout << Color::DIM << "  Thanks for playing.\n\n" << Color::RESET;
            break;
        }

        Display::clearScreen();
        if (!fsmMsg.empty())
            std::cout << "\n" << fsmMsg << "\n";

        printStatus();

        if (!fsm_->canInteract()) {
            printSleepMenu();
            std::cout << Color::BOLD << "\n  Your choice: " << Color::RESET;
            std::string input;
            std::getline(std::cin, input);
            if      (input == "1") handleWait();
            else if (input == "2") { printStatus(); pause(); }
            else if (input == "3") handleQuit();
            else std::cout << Color::YELLOW << "  Invalid choice.\n" << Color::RESET;
        } else {
            printMainMenu();
            std::cout << Color::BOLD << "\n  Your choice: " << Color::RESET;
            std::string input;
            std::getline(std::cin, input);
            if      (input == "1") handleFeed();
            else if (input == "2") handlePlay();
            else if (input == "3") handleRest();
            else if (input == "4") handleUseItem();
            else if (input == "5") handleShop();
            else if (input == "6") handlePetBox();
            else if (input == "7") handleQuit();
            else std::cout << Color::YELLOW << "  Invalid choice.\n" << Color::RESET;
        }
    }

    audio_.stop();
}

bool GameManager::loadGame() {
    std::ifstream file(saveFilePath_);
    if (!file.is_open()) return false;

    std::ostringstream buf;
    buf << file.rdbuf();
    std::string json = buf.str();
    if (json.empty()) return false;

    auto versionStr = [&]() -> std::string {
        auto pos = json.find("\"version\"");
        if (pos == std::string::npos) return "";
        pos = json.find(':', pos + 9);
        if (pos == std::string::npos) return "";
        ++pos;
        while (pos < json.size() && json[pos] == ' ') ++pos;
        auto end = json.find_first_of(",}\n", pos);
        return json.substr(pos, end - pos);
    }();
    if (versionStr != "2") return false;

    auto getInt = [&](const std::string& key) -> int {
        auto pos = json.find("\"" + key + "\"");
        if (pos == std::string::npos) return 0;
        pos = json.find(':', pos + key.size() + 2);
        if (pos == std::string::npos) return 0;
        ++pos;
        while (pos < json.size() && json[pos] == ' ') ++pos;
        auto end = json.find_first_of(",}\n", pos);
        std::string tok = json.substr(pos, end - pos);
        while (!tok.empty() && (tok.back() == ' ' || tok.back() == '\r')) tok.pop_back();
        try { return std::stoi(tok); } catch (...) { return 0; }
    };

    int petCount = getInt("petCount");
    if (petCount <= 0) return false;
    activePetIdx_ = getInt("activePet");

    try {
        for (int i = 0; i < petCount; ++i) {
            std::string prefix = "pet" + std::to_string(i) + "_";
            pets_.push_back(std::make_unique<Pet>(Pet::deserializeFromJson(json, prefix)));
        }
    } catch (const std::exception& ex) {
        std::cerr << Color::BRIGHT_RED << "  Save corrupted: " << ex.what()
                  << "\n  Starting fresh.\n" << Color::RESET;
        pets_.clear();
        return false;
    }

    inventory_.loadFromJson(json, Shop::catalog());

    if (activePetIdx_ < 0 || activePetIdx_ >= static_cast<int>(pets_.size()))
        activePetIdx_ = 0;

    long long mins = minutesSinceLastSave();
    if (mins > 0) {
        std::cout << Color::BRIGHT_YELLOW
                  << "\nWelcome back! Checking on " << activePet().getName() << "...\n"
                  << Color::RESET;
        std::cout << activePet().applyOfflineDecay(mins);
        pause();
    } else {
        std::cout << Color::BRIGHT_GREEN
                  << "\nWelcome back, " << activePet().getName() << "!\n" << Color::RESET;
    }
    return true;
}

bool GameManager::loadLegacyGame() {
    std::ifstream file(legacySavePath_);
    if (!file.is_open()) return false;

    std::ostringstream buf;
    buf << file.rdbuf();
    std::string json = buf.str();
    if (json.empty()) return false;

    try {
        pets_.push_back(std::make_unique<Pet>(Pet::deserialize(json)));
    } catch (const std::exception& ex) {
        std::cerr << Color::BRIGHT_RED
                  << "  Legacy save corrupted: " << ex.what() << "\n" << Color::RESET;
        return false;
    }

    activePetIdx_ = 0;
    std::cout << Color::BRIGHT_CYAN
              << "\n  Migrated save from v1. Welcome back, "
              << activePet().getName() << "!\n" << Color::RESET;

    long long mins = minutesSinceLastSave();
    if (mins > 0)
        std::cout << activePet().applyOfflineDecay(mins);
    pause();
    return true;
}

void GameManager::saveGame() {
    activePet().stampSaveTime();

    std::ofstream file(saveFilePath_);
    if (!file.is_open()) {
        std::cerr << Color::BRIGHT_RED
                  << "  Warning: could not write save file at " << saveFilePath_
                  << "\n" << Color::RESET;
        return;
    }

    std::ostringstream ss;
    ss << "{\n"
       << "  \"version\": 2,\n"
       << "  \"activePet\": " << activePetIdx_ << ",\n"
       << "  \"petCount\": "  << pets_.size()  << ",\n";

    for (int i = 0; i < static_cast<int>(pets_.size()); ++i) {
        std::string prefix = "pet" + std::to_string(i) + "_";
        pets_[static_cast<size_t>(i)]->serializeToJson(ss, prefix);
    }

    inventory_.appendToJson(ss);

    std::string result = ss.str();
    auto lastComma = result.rfind(",\n");
    if (lastComma != std::string::npos)
        result.replace(lastComma, 2, "\n");
    result += "}\n";

    file << result;
    std::cout << Color::DIM << "\n  Game saved.\n" << Color::RESET;
}

long long GameManager::minutesSinceLastSave() const {
    using namespace std::chrono;
    auto last = system_clock::from_time_t(activePet().getLastSaved());
    auto now  = system_clock::now();
    return duration_cast<minutes>(now - last).count();
}

void GameManager::printStatus() const {
    const Pet& pet = activePet();
    Display::printBanner("  " + pet.getName() + "  [" + pet.getStageName() + "]  ");

    std::cout << "\n" << Display::getAsciiArt(pet.getStage(), pet.getCurrentMood()) << "\n";

    const auto& s = pet.getStats();
    const auto& g = pet.getGenome();
    std::cout << Display::statLine("Hunger",    s.hunger,    g.maxHunger)    << "\n"
              << Display::statLine("Happiness", s.happiness, g.maxHappiness) << "\n"
              << Display::statLine("Energy",    s.energy,    g.maxEnergy)    << "\n"
              << Display::statLine("Health",    s.health,    g.maxHealth)    << "\n";

    std::cout << Color::BOLD << "\n  State:  " << Color::RESET
              << Color::BRIGHT_CYAN << fsm_->getStateName() << Color::RESET
              << Color::DIM << "  (" << fsm_->getStateDesc() << ")" << Color::RESET << "\n";

    if (pet.isSick())
        std::cout << Color::BRIGHT_RED << Color::BOLD
                  << "  !! " << pet.getName() << " is SICK !!\n" << Color::RESET;

    int next = pet.nextEvolutionAt();
    if (next > 0) {
        int count = pet.getInteractionCount();
        std::cout << Color::DIM << "  Evolution: " << count << "/" << next
                  << " interactions";
        int v = pet.getGenome().evolutionVariant;
        if (v == 1)      std::cout << " (Quick lineage)";
        else if (v == 2) std::cout << " (Robust lineage)";
        std::cout << "\n" << Color::RESET;
    } else {
        std::cout << Color::BRIGHT_MAGENTA << "  " << pet.getName()
                  << " has reached full evolution!\n" << Color::RESET;
    }

    std::cout << Color::BRIGHT_YELLOW << "  Coins: " << inventory_.getCurrency()
              << Color::RESET << "   ";
    if (audio_.isPlaying())
        std::cout << Color::DIM << "  Playing: " << audio_.getCurrentTrack() << Color::RESET;
    std::cout << "\n";

    if (pets_.size() > 1)
        std::cout << Color::DIM << "  Pet Box: " << pets_.size() << " pets owned  "
                  << "(active: " << activePet().getName() << ")\n" << Color::RESET;

    Display::printDivider();
}

void GameManager::printMainMenu() const {
    std::cout << Color::BOLD
              << "\n  1) Feed   2) Play   3) Rest   4) Use Item\n"
              << "  5) Shop   6) Pet Box\n"
              << "  7) Quit\n"
              << Color::RESET;
}

void GameManager::printSleepMenu() const {
    std::cout << Color::BLUE << Color::BOLD
              << "\n  " << activePet().getName() << " is sleeping...\n"
              << "  (Normal interaction is locked until energy recovers.)\n\n"
              << Color::RESET << Color::BOLD
              << "  1) Wait (rest 15 min)   2) Status   3) Quit\n"
              << Color::RESET;
}

void GameManager::handleFeed() {
    std::cout << Color::BRIGHT_GREEN << "\n  Nom nom nom! " << activePet().getName()
              << " is eating...\n" << Color::RESET;
    activePet().feed(30);
    std::string ev = postActionTick();
    if (!ev.empty()) std::cout << "\n" << ev << "\n";
    pause();
}

void GameManager::handlePlay() {
    fsm_->transitionTo(FSMState::Playing);
    std::cout << Color::BRIGHT_YELLOW << "\n  Time to play!\n" << Color::RESET;

    int bonus = Minigame::runGuessingGame();
    activePet().receiveHappinessBoost(bonus);

    bool won = (bonus >= 30);
    int coins = won ? 50 : 15;
    inventory_.addCurrency(coins);
    std::cout << (won ? Color::BRIGHT_GREEN : Color::YELLOW)
              << "  Earned " << coins << " coins!\n" << Color::RESET;

    std::string ev = postActionTick();
    if (!ev.empty()) std::cout << "\n" << ev << "\n";
    pause();
}

void GameManager::handleRest() {
    std::cout << Color::BLUE << "\n  " << activePet().getName()
              << " is resting...\n  z Z z Z\n" << Color::RESET;
    activePet().rest();
    std::string ev = postActionTick();
    if (!ev.empty()) std::cout << "\n" << ev << "\n";
    pause();
}

void GameManager::handleUseItem() {
    auto items = inventory_.getAllItems();
    if (items.empty()) {
        std::cout << Color::YELLOW
                  << "\n  Your inventory is empty. Visit the Shop (option 5)!\n"
                  << Color::RESET;
        pause();
        return;
    }

    Display::clearScreen();
    Display::printBanner("  INVENTORY  ");
    std::cout << Color::BRIGHT_YELLOW << "\n  Coins: " << inventory_.getCurrency()
              << "\n\n" << Color::RESET;

    int idx = 1;
    for (const auto& [item, qty] : items) {
        const char* typeColor =
            (item->getType() == "Food")     ? Color::BRIGHT_GREEN  :
            (item->getType() == "Medicine") ? Color::BRIGHT_RED    :
                                              Color::BRIGHT_MAGENTA;
        std::cout << "  " << Color::BOLD << idx++ << Color::RESET << ")  "
                  << std::left << std::setw(18) << item->getName()
                  << typeColor << std::setw(12) << item->getType() << Color::RESET
                  << " x" << qty << "\n"
                  << Color::DIM << "        " << item->getDescription()
                  << Color::RESET << "\n";
    }

    std::cout << "\n  " << Color::BOLD << idx << Color::RESET << ") Back\n";
    std::cout << Color::BOLD << "\n  Choose item to use: " << Color::RESET;

    std::string input;
    std::getline(std::cin, input);
    int choice = 0;
    try { choice = std::stoi(input); } catch (...) {}

    if (choice < 1 || choice > static_cast<int>(items.size())) return;

    const auto& [chosenItem, qty] = items[static_cast<size_t>(choice) - 1];
    inventory_.removeItem(chosenItem->getId(), 1);

    std::string result = chosenItem->apply(activePet());
    std::cout << Color::BRIGHT_GREEN << "\n  " << result << "\n" << Color::RESET;

    std::string ev = postActionTick();
    if (!ev.empty()) std::cout << "\n" << ev << "\n";
    pause();
}

void GameManager::handleShop() {
    Shop::run(inventory_, activePet().getName());
}

void GameManager::handlePetBox() {
    while (true) {
        Display::clearScreen();
        Display::printBanner("  PET BOX  ");

        std::cout << "\n";
        for (int i = 0; i < static_cast<int>(pets_.size()); ++i) {
            const auto& p = *pets_[static_cast<size_t>(i)];
            const char* stageColor =
                (p.getStage() == EvolutionStage::Adult) ? Color::BRIGHT_MAGENTA :
                (p.getStage() == EvolutionStage::Teen)  ? Color::BRIGHT_CYAN    :
                (p.getStage() == EvolutionStage::Child) ? Color::BRIGHT_GREEN   :
                                                          Color::BRIGHT_YELLOW;
            std::cout << "  " << Color::BOLD << (i + 1) << Color::RESET << ") "
                      << stageColor << p.getName() << Color::RESET
                      << " [" << p.getStageName() << "]"
                      << (i == activePetIdx_ ? Color::BRIGHT_CYAN + std::string(" ← Active") + Color::RESET : "")
                      << (!p.isAlive() ? Color::BRIGHT_RED + std::string(" (deceased)") + Color::RESET : "")
                      << "\n"
                      << Color::DIM << "     HP:" << static_cast<int>(p.getStats().health)
                      << " | Evo:" << p.getStageName()
                      << " | Interactions:" << p.getInteractionCount()
                      << " | Variant:" << p.getGenome().evolutionVariant
                      << Color::RESET << "\n";
        }

        std::cout << "\n"
                  << Color::BOLD
                  << "  S) Switch active pet\n"
                  << "  B) Breed two Adults\n"
                  << "  0) Back\n"
                  << Color::RESET;

        std::cout << Color::BOLD << "\n  Choice: " << Color::RESET;
        std::string input;
        std::getline(std::cin, input);

        if (input == "0") break;

        if (input == "S" || input == "s") {
            if (pets_.size() < 2) {
                std::cout << Color::YELLOW << "  Only one pet — nothing to switch to.\n" << Color::RESET;
                pause();
                continue;
            }
            std::cout << "  Switch to which pet? (1-" << pets_.size() << "): ";
            std::getline(std::cin, input);
            int idx = 0;
            try { idx = std::stoi(input) - 1; } catch (...) {}
            if (idx < 0 || idx >= static_cast<int>(pets_.size())) {
                std::cout << Color::YELLOW << "  Invalid choice.\n" << Color::RESET;
                pause();
                continue;
            }
            if (!pets_[static_cast<size_t>(idx)]->isAlive()) {
                std::cout << Color::BRIGHT_RED << "  That pet has passed away.\n" << Color::RESET;
                pause();
                continue;
            }
            activePetIdx_ = idx;
            rebuildFSM();
            std::cout << Color::BRIGHT_GREEN
                      << "  Now caring for " << activePet().getName() << "!\n" << Color::RESET;
            pause();
            continue;
        }

        if (input == "B" || input == "b") {
            std::vector<int> adultIndices;
            for (int i = 0; i < static_cast<int>(pets_.size()); ++i) {
                if (pets_[static_cast<size_t>(i)]->getStage() == EvolutionStage::Adult &&
                    pets_[static_cast<size_t>(i)]->isAlive())
                    adultIndices.push_back(i);
            }

            if (adultIndices.size() < 2) {
                std::cout << Color::YELLOW
                          << "\n  You need at least 2 Adult pets to breed.\n"
                          << "  Adult stage requires reaching the final evolution.\n"
                          << Color::RESET;
                pause();
                continue;
            }

            if (static_cast<int>(pets_.size()) >= MAX_PETS) {
                std::cout << Color::YELLOW
                          << "\n  Pet Box is full! (" << MAX_PETS << "/" << MAX_PETS << ")\n"
                          << Color::RESET;
                pause();
                continue;
            }

            std::cout << "\n  Adult pets available for breeding:\n";
            for (int ai : adultIndices)
                std::cout << "    " << (ai + 1) << ") " << pets_[static_cast<size_t>(ai)]->getName() << "\n";

            std::cout << "  Select Parent A (number): ";
            std::getline(std::cin, input);
            int idxA = 0;
            try { idxA = std::stoi(input) - 1; } catch (...) {}

            std::cout << "  Select Parent B (number): ";
            std::getline(std::cin, input);
            int idxB = 0;
            try { idxB = std::stoi(input) - 1; } catch (...) {}

            if (idxA == idxB ||
                idxA < 0 || idxA >= static_cast<int>(pets_.size()) ||
                idxB < 0 || idxB >= static_cast<int>(pets_.size())) {
                std::cout << Color::YELLOW << "  Invalid parents.\n" << Color::RESET;
                pause();
                continue;
            }

            if (pets_[static_cast<size_t>(idxA)]->getStage() != EvolutionStage::Adult ||
                pets_[static_cast<size_t>(idxB)]->getStage() != EvolutionStage::Adult) {
                std::cout << Color::YELLOW << "  Both parents must be Adult stage.\n" << Color::RESET;
                pause();
                continue;
            }

            std::cout << "  Name the offspring: ";
            std::getline(std::cin, input);
            if (input.empty()) input = "Junior";

            auto offspring = Genetics::breed(*pets_[static_cast<size_t>(idxA)],
                                             *pets_[static_cast<size_t>(idxB)],
                                             input);
            if (!offspring) {
                std::cout << Color::YELLOW << "  Breeding failed.\n" << Color::RESET;
                pause();
                continue;
            }

            const Genome& g = offspring->getGenome();
            std::cout << Color::BRIGHT_MAGENTA << Color::BOLD
                      << "\n  A new pet was born: " << offspring->getName() << "!\n"
                      << Color::RESET
                      << Color::DIM
                      << "  Genome — Max Hunger:"    << static_cast<int>(g.maxHunger)
                      << " / Max Health:"             << static_cast<int>(g.maxHealth)
                      << " / Decay Resistance:"       << std::fixed << std::setprecision(2)
                                                      << g.decayResistance
                      << " / Evo Variant:"            << g.evolutionVariant
                      << "\n" << Color::RESET;

            pets_.push_back(std::move(offspring));
            saveGame();
            pause();
            continue;
        }
    }
}

void GameManager::handleWait() {
    Pet& pet = activePet();
    std::cout << Color::BLUE << "\n  Time passes... z Z z Z\n" << Color::RESET;

    pet.addEnergy(15.0f);
    pet.applyOfflineDecay(15);

    std::string fsmMsg = fsm_->tick();
    if (!fsmMsg.empty())
        std::cout << "\n" << fsmMsg << "\n";

    saveGame();
    pause();
}

void GameManager::handleQuit() {
    saveGame();
    std::cout << Color::BRIGHT_CYAN << "  Goodbye! See you soon!\n" << Color::RESET;
    running_ = false;
}

std::string GameManager::postActionTick() {
    Pet& pet = activePet();
    pet.applyInteractionDecay();
    pet.incrementInteractions();

    std::string output;

    bool evolved = pet.checkAndEvolve();
    if (evolved) {
        output += Color::BRIGHT_MAGENTA + std::string(Color::BOLD) +
                  "\n  *** " + pet.getName() + " EVOLVED into a " +
                  pet.getStageName() + "! ***\n" + Color::RESET;
        output += Display::getAsciiArt(pet.getStage(), pet.getCurrentMood());
    }

    std::string event = pet.triggerRandomEvent();
    if (!event.empty())
        output += "\n" + event;

    std::string fsmMsg = fsm_->tick();
    if (!fsmMsg.empty())
        output += "\n" + fsmMsg;

    saveGame();
    return output;
}
