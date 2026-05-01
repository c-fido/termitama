#include "Pet.h"
#include <algorithm>
#include <cmath>
#include <cstdlib>
#include <sstream>
#include <stdexcept>
#include <string>


static float clamp(float v, float lo, float hi) {
    return std::max(lo, std::min(v, hi));
}

static const int EVO_THRESHOLDS[3][3] = {
    {15, 40, 80},
    {10, 25, 50},
    {20, 55, 110},
};

std::string Pet::jsonGet(const std::string& json, const std::string& key) {
    std::string needle = "\"" + key + "\"";
    auto pos = json.find(needle);
    if (pos == std::string::npos) return "";
    pos = json.find(':', pos + needle.size());
    if (pos == std::string::npos) return "";
    ++pos;
    while (pos < json.size() &&
           (json[pos] == ' ' || json[pos] == '\n' || json[pos] == '\r')) ++pos;
    if (pos >= json.size()) return "";

    if (json[pos] == '"') {
        auto end = json.find('"', pos + 1);
        return (end == std::string::npos) ? "" : json.substr(pos + 1, end - pos - 1);
    }
    auto end = json.find_first_of(",}\n", pos);
    std::string tok = (end == std::string::npos)
                    ? json.substr(pos)
                    : json.substr(pos, end - pos);
    while (!tok.empty() &&
           (tok.back() == ' ' || tok.back() == '\r' || tok.back() == '\n'))
        tok.pop_back();
    return tok;
}

Pet::Pet(const std::string& name) : name_(name) {
    lastSaved_ = std::time(nullptr);
}

Pet::DecayRates Pet::getDecayRates() const {
    DecayRates r;
    float mult = genome_.decayResistance;
    if (stage_ == EvolutionStage::Adult) mult *= 0.7f;
    r.hungerPerMin    *= mult;
    r.happinessPerMin *= mult;
    r.energyPerMin    *= mult;
    r.healthPerMin    *= mult;
    return r;
}

void Pet::clampStats() {
    stats_.hunger    = clamp(stats_.hunger,    0.0f, genome_.maxHunger);
    stats_.happiness = clamp(stats_.happiness, 0.0f, genome_.maxHappiness);
    stats_.energy    = clamp(stats_.energy,    0.0f, genome_.maxEnergy);
    stats_.health    = clamp(stats_.health,    0.0f, genome_.maxHealth);
}

void Pet::applyDecayForDuration(float minutes) {
    DecayRates r = getDecayRates();

    stats_.hunger    -= r.hungerPerMin    * minutes;
    stats_.happiness -= r.happinessPerMin * minutes;
    stats_.energy    -= r.energyPerMin    * minutes;

    bool neglected = (stats_.hunger < 20.0f || stats_.happiness < 20.0f ||
                      stats_.energy < 20.0f);
    float healthMult = isSick_ ? 2.0f : 1.0f;
    if (neglected || isSick_)
        stats_.health -= r.healthPerMin * minutes * healthMult;

    clampStats();
}

std::string Pet::applyOfflineDecay(long long minutesPassed) {
    const long long MAX_OFFLINE = 720;
    long long effective = std::min(minutesPassed, MAX_OFFLINE);

    float before_health = stats_.health;
    float before_hunger = stats_.hunger;

    applyDecayForDuration(static_cast<float>(effective));

    std::ostringstream msg;
    msg << "  " << name_ << " was alone for " << minutesPassed << " minute(s).";
    if (minutesPassed > MAX_OFFLINE) msg << " (decay capped at 12 hours)";
    msg << "\n";

    if (stats_.hunger < before_hunger - 10.0f)
        msg << "  " << name_ << " is getting hungry...\n";
    if (stats_.health < before_health)
        msg << "  Health dropped by "
            << static_cast<int>(before_health - stats_.health) << " points.\n";
    if (!isAlive())
        msg << "  Oh no! " << name_ << " didn't survive the absence.\n";

    return msg.str();
}

void Pet::applyInteractionDecay() {
    applyDecayForDuration(5.0f);
}

void Pet::feed(int amount) {
    applyFoodEffect(amount, static_cast<int>(amount * 0.2f));
}

void Pet::applyFoodEffect(int hungerAmount, int happinessBonus) {
    stats_.hunger    = clamp(stats_.hunger    + static_cast<float>(hungerAmount),
                             0.0f, genome_.maxHunger);
    stats_.happiness = clamp(stats_.happiness + static_cast<float>(happinessBonus),
                             0.0f, genome_.maxHappiness);
}

void Pet::applyMedicine(int healthAmount, bool curesSickness) {
    stats_.health = clamp(stats_.health + static_cast<float>(healthAmount),
                          0.0f, genome_.maxHealth);
    if (curesSickness) isSick_ = false;
}

void Pet::receiveHappinessBoost(int amount) {
    stats_.happiness = clamp(stats_.happiness + static_cast<float>(amount),
                             0.0f, genome_.maxHappiness);
    stats_.energy    = clamp(stats_.energy    - static_cast<float>(amount) * 0.3f,
                             0.0f, genome_.maxEnergy);
}

void Pet::rest() {
    stats_.energy    = clamp(stats_.energy    + 40.0f, 0.0f, genome_.maxEnergy);
    stats_.happiness = clamp(stats_.happiness + 5.0f,  0.0f, genome_.maxHappiness);
    applyDecayForDuration(20.0f);
}

void Pet::addEnergy(float amount) {
    stats_.energy = clamp(stats_.energy + amount, 0.0f, genome_.maxEnergy);
}

bool Pet::checkAndEvolve() {
    if (stats_.health < 50.0f) return false;

    int threshold = nextEvolutionAt();
    if (threshold < 0 || interactionCount_ < threshold) return false;

    using E = EvolutionStage;
    switch (stage_) {
        case E::Baby:  stage_ = E::Child; break;
        case E::Child: stage_ = E::Teen;  break;
        case E::Teen:  stage_ = E::Adult; break;
        case E::Adult: break;
    }
    return true;
}

int Pet::nextEvolutionAt() const {
    using E = EvolutionStage;
    int stageIdx = 0;
    switch (stage_) {
        case E::Baby:  stageIdx = 0; break;
        case E::Child: stageIdx = 1; break;
        case E::Teen:  stageIdx = 2; break;
        case E::Adult: return -1;
    }
    int v = std::max(0, std::min(genome_.evolutionVariant, 2));
    return EVO_THRESHOLDS[v][stageIdx];
}

std::string Pet::getStageName() const {
    using E = EvolutionStage;
    switch (stage_) {
        case E::Baby:  return "Baby";
        case E::Child: return "Child";
        case E::Teen:  return "Teen";
        case E::Adult: return "Adult";
    }
    return "Unknown";
}

Mood Pet::getCurrentMood() const {
    if (isSick_)               return Mood::Sick;
    if (stats_.energy < 15.0f) return Mood::Sleeping;
    float avg = (stats_.hunger + stats_.happiness + stats_.health) / 3.0f;
    if (avg >= 70.0f)          return Mood::Happy;
    if (avg >= 40.0f)          return Mood::Neutral;
    return Mood::Sad;
}

std::string Pet::triggerRandomEvent() {
    if ((std::rand() % 100) >= 8) return "";

    int roll = std::rand() % 8;
    switch (roll) {
        case 0:
            stats_.hunger = clamp(stats_.hunger + 20.0f, 0.0f, genome_.maxHunger);
            return Color::BRIGHT_GREEN + std::string("EVENT: ") + name_ +
                   " found a mystery snack! (+20 Hunger)" + Color::RESET;
        case 1:
            stats_.happiness = clamp(stats_.happiness + 15.0f, 0.0f, genome_.maxHappiness);
            return Color::BRIGHT_YELLOW + std::string("EVENT: It's a sunny day! ") +
                   name_ + " is cheerful! (+15 Happiness)" + Color::RESET;
        case 2:
            if (!isSick_) {
                isSick_ = true;
                return Color::BRIGHT_RED + std::string("EVENT: Oh no! ") + name_ +
                       " caught a cold! (Health drains faster)" + Color::RESET;
            }
            isSick_ = false;
            stats_.health = clamp(stats_.health + 10.0f, 0.0f, genome_.maxHealth);
            return Color::BRIGHT_GREEN + std::string("EVENT: ") + name_ +
                   " recovered from illness! (+10 Health)" + Color::RESET;
        case 3:
            stats_.energy = clamp(stats_.energy - 20.0f, 0.0f, genome_.maxEnergy);
            return Color::BLUE + std::string("EVENT: ") + name_ +
                   " had a nightmare! (-20 Energy)" + Color::RESET;
        case 4:
            stats_.happiness = clamp(stats_.happiness + 20.0f, 0.0f, genome_.maxHappiness);
            return Color::BRIGHT_CYAN + std::string("EVENT: ") + name_ +
                   " made a new friend! (+20 Happiness)" + Color::RESET;
        case 5:
            stats_.happiness = clamp(stats_.happiness - 15.0f, 0.0f, genome_.maxHappiness);
            return Color::YELLOW + std::string("EVENT: ") + name_ +
                   " broke a favourite toy. (-15 Happiness)" + Color::RESET;
        case 6:
            stats_.health = clamp(stats_.health + 15.0f, 0.0f, genome_.maxHealth);
            return Color::BRIGHT_GREEN + std::string("EVENT: ") + name_ +
                   " drank a health potion! (+15 Health)" + Color::RESET;
        case 7:
            stats_.hunger    = clamp(stats_.hunger    + 10.0f, 0.0f, genome_.maxHunger);
            stats_.happiness = clamp(stats_.happiness + 10.0f, 0.0f, genome_.maxHappiness);
            stats_.energy    = clamp(stats_.energy    + 10.0f, 0.0f, genome_.maxEnergy);
            return Color::BRIGHT_MAGENTA + std::string("EVENT: A shooting star! ") +
                   name_ + " feels blessed! (+10 all stats)" + Color::RESET;
        default: return "";
    }
}

void Pet::serializeToJson(std::ostringstream& ss, const std::string& p) const {
    auto k = [&](const std::string& key) { return "\"" + p + key + "\""; };
    ss << "  " << k("name")          << ": \"" << name_             << "\",\n"
       << "  " << k("hunger")        << ": "   << stats_.hunger     << ",\n"
       << "  " << k("happiness")     << ": "   << stats_.happiness  << ",\n"
       << "  " << k("energy")        << ": "   << stats_.energy     << ",\n"
       << "  " << k("health")        << ": "   << stats_.health     << ",\n"
       << "  " << k("stage")         << ": "   << static_cast<int>(stage_) << ",\n"
       << "  " << k("interactions")  << ": "   << interactionCount_ << ",\n"
       << "  " << k("isSick")        << ": "   << (isSick_ ? "true" : "false") << ",\n"
       << "  " << k("lastSaved")     << ": "   << static_cast<long long>(lastSaved_) << ",\n"
       << "  " << k("fsmState")      << ": "   << static_cast<int>(fsmState_)  << ",\n"
       << "  " << k("g_maxHunger")   << ": "   << genome_.maxHunger        << ",\n"
       << "  " << k("g_maxHappy")    << ": "   << genome_.maxHappiness     << ",\n"
       << "  " << k("g_maxEnergy")   << ": "   << genome_.maxEnergy        << ",\n"
       << "  " << k("g_maxHealth")   << ": "   << genome_.maxHealth        << ",\n"
       << "  " << k("g_decayRes")    << ": "   << genome_.decayResistance  << ",\n"
       << "  " << k("g_evoVariant")  << ": "   << genome_.evolutionVariant << ",\n";
}

Pet Pet::deserializeFromJson(const std::string& json, const std::string& p) {
    auto g = [&](const std::string& key) { return jsonGet(json, p + key); };

    std::string name = g("name");
    if (name.empty())
        throw std::runtime_error("Pet deserialize: missing '" + p + "name'");

    Pet pet(name);
    try {
        pet.stats_.hunger       = std::stof(g("hunger"));
        pet.stats_.happiness    = std::stof(g("happiness"));
        pet.stats_.energy       = std::stof(g("energy"));
        pet.stats_.health       = std::stof(g("health"));
        pet.stage_              = static_cast<EvolutionStage>(std::stoi(g("stage")));
        pet.interactionCount_   = std::stoi(g("interactions"));
        pet.isSick_             = (g("isSick") == "true");
        pet.lastSaved_          = static_cast<std::time_t>(std::stoll(g("lastSaved")));
        pet.fsmState_           = static_cast<FSMState>(std::stoi(g("fsmState")));

        auto parseFloat = [&](const std::string& key, float def) -> float {
            std::string tok = g(key);
            return tok.empty() ? def : std::stof(tok);
        };
        auto parseInt = [&](const std::string& key, int def) -> int {
            std::string tok = g(key);
            return tok.empty() ? def : std::stoi(tok);
        };
        pet.genome_.maxHunger        = parseFloat("g_maxHunger",  100.0f);
        pet.genome_.maxHappiness     = parseFloat("g_maxHappy",   100.0f);
        pet.genome_.maxEnergy        = parseFloat("g_maxEnergy",  100.0f);
        pet.genome_.maxHealth        = parseFloat("g_maxHealth",  100.0f);
        pet.genome_.decayResistance  = parseFloat("g_decayRes",   1.0f);
        pet.genome_.evolutionVariant = parseInt  ("g_evoVariant", 0);
    } catch (const std::exception& e) {
        throw std::runtime_error(std::string("Pet parse error: ") + e.what());
    }
    pet.clampStats();
    return pet;
}

std::string Pet::serialize() const {
    std::ostringstream ss;
    ss << "{\n";
    serializeToJson(ss, "");
    ss.str("");
    ss.clear();
    ss << "{\n"
       << "  \"name\": \""            << name_             << "\",\n"
       << "  \"hunger\": "            << stats_.hunger     << ",\n"
       << "  \"happiness\": "         << stats_.happiness  << ",\n"
       << "  \"energy\": "            << stats_.energy     << ",\n"
       << "  \"health\": "            << stats_.health     << ",\n"
       << "  \"stage\": "             << static_cast<int>(stage_) << ",\n"
       << "  \"interactionCount\": "  << interactionCount_ << ",\n"
       << "  \"isSick\": "            << (isSick_ ? "true" : "false") << ",\n"
       << "  \"lastSaved\": "         << static_cast<long long>(lastSaved_) << "\n"
       << "}\n";
    return ss.str();
}

Pet Pet::deserialize(const std::string& json) {
    std::string name = jsonGet(json, "name");
    if (name.empty())
        throw std::runtime_error("Invalid save file: missing name");

    Pet pet(name);
    try {
        pet.stats_.hunger       = std::stof(jsonGet(json, "hunger"));
        pet.stats_.happiness    = std::stof(jsonGet(json, "happiness"));
        pet.stats_.energy       = std::stof(jsonGet(json, "energy"));
        pet.stats_.health       = std::stof(jsonGet(json, "health"));
        pet.stage_              = static_cast<EvolutionStage>(std::stoi(jsonGet(json, "stage")));
        pet.interactionCount_   = std::stoi(jsonGet(json, "interactionCount"));
        pet.isSick_             = (jsonGet(json, "isSick") == "true");
        pet.lastSaved_          = static_cast<std::time_t>(
                                      std::stoll(jsonGet(json, "lastSaved")));
    } catch (const std::exception& e) {
        throw std::runtime_error(std::string("Save file parse error: ") + e.what());
    }
    pet.clampStats();
    return pet;
}
