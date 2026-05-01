#pragma once
#include "Pet.h"
#include "PetFSM.h"
#include "Inventory.h"
#include "AudioManager.h"
#include <string>
#include <vector>
#include <memory>

class GameManager {
public:
    GameManager(const std::string& saveFile, const std::string& legacySaveFile);

    void run();

    static constexpr int MAX_PETS = 6;

private:
    std::vector<std::unique_ptr<Pet>> pets_;
    int                               activePetIdx_ = 0;
    std::unique_ptr<PetFSM>           fsm_;
    Inventory                         inventory_;
    AudioManager                      audio_;
    std::string                       saveFilePath_;
    std::string                       legacySavePath_;
    bool                              running_ = true;

    Pet& activePet();
    const Pet& activePet() const;
    void rebuildFSM();

    bool loadGame();
    bool loadLegacyGame();
    void saveGame();

    long long minutesSinceLastSave() const;

    void printStatus()     const;
    void printMainMenu()   const;
    void printSleepMenu()  const;

    void handleFeed();
    void handlePlay();
    void handleRest();
    void handleUseItem();
    void handleShop();
    void handlePetBox();
    void handleWait();
    void handleQuit();

    std::string postActionTick();

    static void pause(const std::string& msg = "");
};
