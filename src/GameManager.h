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
    ~GameManager();

    static constexpr int MAX_PETS = 6;

    bool load(std::string& outMsg);
    void newGame(const std::string& petName);

    const Pet&       activePet()    const;
    const Pet&       getActivePet() const { return activePet(); }
    const Inventory& inventory()    const { return inventory_; }
    bool             canInteract()  const;
    std::string      fsmStateName() const;
    std::string      fsmStateDesc() const;

    int        petCount()       const { return static_cast<int>(pets_.size()); }
    const Pet& getPet(int i)    const;
    int        activePetIndex() const { return activePetIdx_; }

    bool isRunning() const { return running_; }

    std::string actionFeed();
    std::string actionPlay(int happinessBonus);
    std::string actionRest();
    std::string actionUseItem(int itemIdx);
    std::string actionPurchase(const std::string& itemId);
    std::string actionWait();
    std::string actionAdoptPet(const std::string& name);
    std::string actionRenamePet(const std::string& name);
    std::string actionSwitchPet(int idx);
    std::string actionBreedPets(int idxA, int idxB, const std::string& offspringName);
    void        actionQuit();

    std::string tick();

    void save();

private:
    std::vector<std::unique_ptr<Pet>> pets_;
    int                               activePetIdx_    = 0;
    std::unique_ptr<PetFSM>           fsm_;
    Inventory                         inventory_;
    AudioManager                      audio_;
    std::string                       saveFilePath_;
    std::string                       legacySavePath_;
    bool                              running_         = true;
    int                               ticksSinceSave_  = 0;

    Pet&       activePet();
    void       rebuildFSM();
    bool       loadV2(std::string& outMsg);
    bool       loadLegacy(std::string& outMsg);
    std::string postActionTick();
    long long  minutesSinceLastSave() const;

    static std::string stripAnsi(const std::string& s);
};
