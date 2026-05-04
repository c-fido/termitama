#include "GameManager.h"
#include "Shop.h"
#include "Genetics.h"
#include <chrono>
#include <fstream>
#include <sstream>
#include <iomanip>
#include <stdexcept>
#include <algorithm>

std::string GameManager::stripAnsi(const std::string& s) {
    std::string out;
    out.reserve(s.size());
    for (size_t i = 0; i < s.size(); ++i) {
        if (s[i] == '\033' && i + 1 < s.size() && s[i + 1] == '[') {
            i += 2;
            while (i < s.size() && s[i] != 'm') ++i;
        } else {
            out += s[i];
        }
    }
    return out;
}

GameManager::GameManager(const std::string& saveFile,
                         const std::string& legacySaveFile)
    : saveFilePath_(saveFile), legacySavePath_(legacySaveFile)
{}

GameManager::~GameManager() {
    audio_.stop();
}

Pet& GameManager::activePet() {
    return *pets_[static_cast<size_t>(activePetIdx_)];
}

const Pet& GameManager::activePet() const {
    return *pets_[static_cast<size_t>(activePetIdx_)];
}

const Pet& GameManager::getPet(int i) const {
    return *pets_[static_cast<size_t>(i)];
}

void GameManager::rebuildFSM() {
    fsm_ = std::make_unique<PetFSM>(activePet());
    fsm_->syncFromPet();
}

bool GameManager::canInteract() const {
    return fsm_ && fsm_->canInteract();
}

std::string GameManager::fsmStateName() const {
    return fsm_ ? fsm_->getStateName() : "";
}

std::string GameManager::fsmStateDesc() const {
    return fsm_ ? fsm_->getStateDesc() : "";
}

long long GameManager::minutesSinceLastSave() const {
    using namespace std::chrono;
    auto last = system_clock::from_time_t(activePet().getLastSaved());
    auto now  = system_clock::now();
    return duration_cast<minutes>(now - last).count();
}

static int jsonGetInt(const std::string& json, const std::string& key, int def = 0) {
    auto pos = json.find("\"" + key + "\"");
    if (pos == std::string::npos) return def;
    pos = json.find(':', pos + key.size() + 2);
    if (pos == std::string::npos) return def;
    ++pos;
    while (pos < json.size() && json[pos] == ' ') ++pos;
    auto end = json.find_first_of(",}\n", pos);
    std::string tok = json.substr(pos, end - pos);
    while (!tok.empty() && (tok.back() == ' ' || tok.back() == '\r')) tok.pop_back();
    try { return std::stoi(tok); } catch (...) { return def; }
}

bool GameManager::load(std::string& outMsg) {
    if (loadV2(outMsg))     return true;
    if (loadLegacy(outMsg)) return true;
    return false;
}

void GameManager::newGame(const std::string& petName) {
    std::string name = petName.empty() ? "Pixel" : petName;
    pets_.push_back(std::make_unique<Pet>(name));
    activePetIdx_ = 0;
    rebuildFSM();
    audio_.start();
    save();
}

bool GameManager::loadV2(std::string& outMsg) {
    std::ifstream file(saveFilePath_);
    if (!file.is_open()) return false;

    std::ostringstream buf;
    buf << file.rdbuf();
    std::string json = buf.str();
    if (json.empty()) return false;

    auto vPos = json.find("\"version\"");
    if (vPos == std::string::npos) return false;
    vPos = json.find(':', vPos + 9);
    if (vPos == std::string::npos) return false;
    ++vPos;
    while (vPos < json.size() && json[vPos] == ' ') ++vPos;
    auto vEnd = json.find_first_of(",}\n", vPos);
    std::string vStr = json.substr(vPos, vEnd - vPos);
    while (!vStr.empty() && (vStr.back() == ' ' || vStr.back() == '\r')) vStr.pop_back();
    if (vStr != "2") return false;

    int petCount = jsonGetInt(json, "petCount", 0);
    if (petCount <= 0) return false;
    activePetIdx_ = jsonGetInt(json, "activePet", 0);

    try {
        for (int i = 0; i < petCount; ++i) {
            std::string prefix = "pet" + std::to_string(i) + "_";
            pets_.push_back(std::make_unique<Pet>(Pet::deserializeFromJson(json, prefix)));
        }
    } catch (const std::exception& ex) {
        outMsg = std::string("Save corrupted: ") + ex.what() + ". Starting fresh.";
        pets_.clear();
        return false;
    }

    inventory_.loadFromJson(json, Shop::catalog());

    if (activePetIdx_ < 0 || activePetIdx_ >= static_cast<int>(pets_.size()))
        activePetIdx_ = 0;

    rebuildFSM();
    audio_.start();

    long long mins = minutesSinceLastSave();
    if (mins > 0) {
        std::string decay = stripAnsi(activePet().applyOfflineDecay(mins));
        outMsg = "Welcome back! " + decay;
    } else {
        outMsg = "Welcome back, " + activePet().getName() + "!";
    }
    return true;
}

bool GameManager::loadLegacy(std::string& outMsg) {
    std::ifstream file(legacySavePath_);
    if (!file.is_open()) return false;

    std::ostringstream buf;
    buf << file.rdbuf();
    std::string json = buf.str();
    if (json.empty()) return false;

    try {
        pets_.push_back(std::make_unique<Pet>(Pet::deserialize(json)));
    } catch (const std::exception& ex) {
        outMsg = std::string("Legacy save corrupted: ") + ex.what();
        return false;
    }

    activePetIdx_ = 0;
    rebuildFSM();
    audio_.start();

    long long mins = minutesSinceLastSave();
    std::string decay;
    if (mins > 0)
        decay = stripAnsi(activePet().applyOfflineDecay(mins));

    outMsg = "Migrated v1 save. Welcome back, " + activePet().getName() + "! " + decay;
    return true;
}

void GameManager::save() {
    if (pets_.empty()) return;
    activePet().stampSaveTime();

    std::ofstream file(saveFilePath_);
    if (!file.is_open()) return;

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
}

std::string GameManager::postActionTick() {
    Pet& pet = activePet();
    pet.applyInteractionDecay();
    pet.incrementInteractions();

    std::string out;

    if (pet.checkAndEvolve()) {
        out += "*** " + pet.getName() + " EVOLVED into a " + pet.getStageName() + "! ***";
    }

    std::string ev = stripAnsi(pet.triggerRandomEvent());
    if (!ev.empty()) out += (out.empty() ? "" : "\n") + ev;

    std::string fsmMsg = stripAnsi(fsm_->tick());
    if (!fsmMsg.empty()) out += (out.empty() ? "" : "\n") + fsmMsg;

    save();
    return out;
}

std::string GameManager::actionFeed() {
    if (pets_.empty() || !activePet().isAlive())
        return activePet().getName() + " can't eat right now.";
    activePet().feed(30);
    std::string ev = postActionTick();
    std::string msg = activePet().getName() + " munched happily! (+30 Hunger)";
    return ev.empty() ? msg : msg + "\n" + ev;
}

std::string GameManager::actionPlay(int happinessBonus) {
    if (pets_.empty() || !activePet().isAlive())
        return "Can't play right now.";
    activePet().receiveHappinessBoost(happinessBonus);
    int coins = (happinessBonus >= 30) ? 50 : 15;
    inventory_.addCurrency(coins);
    std::string ev = postActionTick();
    std::string msg = activePet().getName() + " had fun! (+" + std::to_string(happinessBonus) +
                      " Happiness, +" + std::to_string(coins) + " coins)";
    return ev.empty() ? msg : msg + "\n" + ev;
}

std::string GameManager::actionRest() {
    if (pets_.empty() || !activePet().isAlive())
        return "Can't rest right now.";
    activePet().rest();
    std::string ev = postActionTick();
    std::string msg = activePet().getName() + " rested. z Z z Z (+40 Energy)";
    return ev.empty() ? msg : msg + "\n" + ev;
}

std::string GameManager::actionUseItem(int itemIdx) {
    auto items = inventory_.getAllItems();
    if (itemIdx < 0 || itemIdx >= static_cast<int>(items.size()))
        return "Invalid item selection.";
    const auto& [item, qty] = items[static_cast<size_t>(itemIdx)];
    (void)qty;
    inventory_.removeItem(item->getId(), 1);
    std::string result = item->apply(activePet());
    std::string ev = postActionTick();
    return ev.empty() ? result : result + "\n" + ev;
}

std::string GameManager::actionPurchase(const std::string& itemId) {
    auto item = Shop::getItem(itemId);
    if (!item) return "Item not found in shop.";
    if (inventory_.getCurrency() < item->getPrice()) {
        return "Not enough coins! Need " + std::to_string(item->getPrice()) +
               ", have " + std::to_string(inventory_.getCurrency()) + ".";
    }
    inventory_.spendCurrency(item->getPrice());
    inventory_.addItem(item, 1);
    save();
    return "Bought " + item->getName() + " for " + std::to_string(item->getPrice()) +
           " coins. (" + std::to_string(inventory_.getCurrency()) + " remaining)";
}

std::string GameManager::actionWait() {
    Pet& pet = activePet();
    pet.addEnergy(15.0f);
    std::string decay = stripAnsi(pet.applyOfflineDecay(15));
    std::string fsmMsg = stripAnsi(fsm_->tick());
    save();
    std::string msg = "Time passes... z Z z Z (+15 Energy)";
    if (!decay.empty())  msg += "\n" + decay;
    if (!fsmMsg.empty()) msg += "\n" + fsmMsg;
    return msg;
}

std::string GameManager::actionAdoptPet(const std::string& name) {
    if (static_cast<int>(pets_.size()) >= MAX_PETS)
        return "Pet Box is full! (" + std::to_string(MAX_PETS) + "/" +
               std::to_string(MAX_PETS) + ")";
    std::string n = name.empty() ? "Pixel" : name;
    pets_.push_back(std::make_unique<Pet>(n));
    save();
    return "Welcome home, " + n + "!";
}

std::string GameManager::actionRenamePet(const std::string& name) {
    std::string n = name.empty() ? "Pixel" : name;
    if (n == activePet().getName()) return "That's already " + n + "'s name.";
    std::string old = activePet().getName();
    activePet().setName(n);
    save();
    return old + " is now called " + n + "!";
}

std::string GameManager::actionSwitchPet(int idx) {
    if (idx < 0 || idx >= static_cast<int>(pets_.size()))
        return "Invalid pet index.";
    if (!pets_[static_cast<size_t>(idx)]->isAlive())
        return "That pet has passed away.";
    activePetIdx_ = idx;
    rebuildFSM();
    save();
    return "Now caring for " + activePet().getName() + "!";
}

std::string GameManager::actionBreedPets(int idxA, int idxB,
                                         const std::string& offspringName) {
    if (static_cast<int>(pets_.size()) >= MAX_PETS)
        return "Pet Box is full!";
    if (idxA == idxB || idxA < 0 || idxB < 0 ||
        idxA >= static_cast<int>(pets_.size()) ||
        idxB >= static_cast<int>(pets_.size()))
        return "Invalid parents selected.";

    const auto& pA = *pets_[static_cast<size_t>(idxA)];
    const auto& pB = *pets_[static_cast<size_t>(idxB)];
    if (pA.getStage() != EvolutionStage::Adult || pB.getStage() != EvolutionStage::Adult)
        return "Both parents must be Adult stage.";
    if (!pA.isAlive() || !pB.isAlive())
        return "Both parents must be alive.";

    std::string n = offspringName.empty() ? "Junior" : offspringName;
    auto offspring = Genetics::breed(pA, pB, n);
    if (!offspring) return "Breeding failed unexpectedly.";

    const Genome& g = offspring->getGenome();
    std::ostringstream msg;
    msg << "A new pet was born: " << offspring->getName() << "!\n"
        << "Genome — MaxHunger:" << static_cast<int>(g.maxHunger)
        << " MaxHealth:"         << static_cast<int>(g.maxHealth)
        << " DecayRes:"          << std::fixed << std::setprecision(2) << g.decayResistance
        << " Variant:"           << g.evolutionVariant;

    pets_.push_back(std::move(offspring));
    save();
    return msg.str();
}

void GameManager::actionQuit() {
    save();
    running_ = false;
}

std::string GameManager::tick() {
    if (!running_ || pets_.empty()) return "";
    Pet& pet = activePet();
    if (!pet.isAlive()) return "";

    pet.applyRealtimeTick();   // 1 game-minute of decay

    std::string msg = stripAnsi(fsm_->tick());

    ++ticksSinceSave_;
    if (ticksSinceSave_ >= 6) {   // auto-save every ~6 ticks = ~60 seconds
        save();
        ticksSinceSave_ = 0;
    }
    return msg;
}
