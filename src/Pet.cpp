#include "Pet.h"
#include <algorithm>
#include <cmath>
#include <cstdlib>
#include <sstream>
#include <stdexcept>
#include <string>

// ── Helpers ──────────────────────────────────────────────────────────────────

static float clamp(float v, float lo, float hi) {
    return std::max(lo, std::min(v, hi));
}

// Very small JSON extractor for flat objects only. Finds the value for `key`
// in a string like { "key": value } and returns it as a string token.
static std::string jsonGet(const std::string& json, const std::string& key) {
    std::string needle = "\"" + key + "\"";
    auto pos = json.find(needle);
    if (pos == std::string::npos) return "";
    pos = json.find(':', pos + needle.size());
    if (pos == std::string::npos) return "";
    ++pos;
    while (pos < json.size() && (json[pos] == ' ' || json[pos] == '\n' || json[pos] == '\r')) ++pos;
    if (pos >= json.size()) return "";

    if (json[pos] == '"') {
        // String value
        auto end = json.find('"', pos + 1);
        return (end == std::string::npos) ? "" : json.substr(pos + 1, end - pos - 1);
    }
    // Numeric or bool: read until , or }
    auto end = json.find_first_of(",}\n", pos);
    std::string tok = (end == std::string::npos) ? json.substr(pos) : json.substr(pos, end - pos);
    // Trim whitespace
    while (!tok.empty() && (tok.back() == ' ' || tok.back() == '\r' || tok.back() == '\n')) tok.pop_back();
    return tok;
}

// ── Constructor ───────────────────────────────────────────────────────────────

Pet::Pet(const std::string& name) : name_(name) {
    lastSaved_ = std::time(nullptr);
}

// ── Decay rates ───────────────────────────────────────────────────────────────
// Adult pets decay 30% slower as a reward for good long-term care.
Pet::DecayRates Pet::getDecayRates() const {
    DecayRates r;
    if (stage_ == EvolutionStage::Adult) {
        r.hungerPerMin    *= 0.7f;
        r.happinessPerMin *= 0.7f;
        r.energyPerMin    *= 0.7f;
        r.healthPerMin    *= 0.7f;
    }
    return r;
}

void Pet::clampStats() {
    stats_.hunger    = clamp(stats_.hunger,    0.0f, 100.0f);
    stats_.happiness = clamp(stats_.happiness, 0.0f, 100.0f);
    stats_.energy    = clamp(stats_.energy,    0.0f, 100.0f);
    stats_.health    = clamp(stats_.health,    0.0f, 100.0f);
}

// ── applyDecayForDuration ─────────────────────────────────────────────────────
// Core decay math:
//
//   new_stat = old_stat - (rate_per_min * minutes)
//
// Health only drains when another stat is critically low (<20).
// Being sick doubles the health drain rate.
// We cap offline decay at 12 hours so a brief absence isn't catastrophic.
void Pet::applyDecayForDuration(float minutes) {
    DecayRates r = getDecayRates();

    stats_.hunger    -= r.hungerPerMin    * minutes;
    stats_.happiness -= r.happinessPerMin * minutes;
    stats_.energy    -= r.energyPerMin    * minutes;

    // Health drains when the pet is neglected OR sick
    bool neglected = (stats_.hunger < 20.0f || stats_.happiness < 20.0f || stats_.energy < 20.0f);
    float healthMultiplier = (isSick_ ? 2.0f : 1.0f);
    if (neglected || isSick_) {
        stats_.health -= r.healthPerMin * minutes * healthMultiplier;
    }

    clampStats();
}

// ── applyOfflineDecay ─────────────────────────────────────────────────────────
std::string Pet::applyOfflineDecay(long long minutesPassed) {
    // Cap at 12 hours = 720 minutes so the player isn't punished too harshly
    const long long MAX_OFFLINE_MINUTES = 720;
    long long effective = std::min(minutesPassed, MAX_OFFLINE_MINUTES);

    float before_health = stats_.health;
    float before_hunger = stats_.hunger;

    applyDecayForDuration(static_cast<float>(effective));

    std::ostringstream msg;
    msg << "  " << name_ << " was alone for " << minutesPassed << " minute(s).";
    if (minutesPassed > MAX_OFFLINE_MINUTES)
        msg << " (decay capped at 12 hours)";
    msg << "\n";

    if (stats_.hunger < before_hunger - 10.0f)
        msg << "  " << name_ << " is getting hungry...\n";
    if (stats_.health < before_health)
        msg << "  Health dropped by " << static_cast<int>(before_health - stats_.health) << " points.\n";
    if (!isAlive())
        msg << "  Oh no! " << name_ << " didn't survive the absence.\n";

    return msg.str();
}

// ── applyInteractionDecay ─────────────────────────────────────────────────────
// Each interaction represents roughly 5 in-game minutes of time passing.
void Pet::applyInteractionDecay() {
    applyDecayForDuration(5.0f);
}

// ── feed ─────────────────────────────────────────────────────────────────────
void Pet::feed(int amount) {
    stats_.hunger    = clamp(stats_.hunger    + static_cast<float>(amount), 0.0f, 100.0f);
    stats_.happiness = clamp(stats_.happiness + static_cast<float>(amount) * 0.2f, 0.0f, 100.0f);
}

// ── receiveHappinessBoost ─────────────────────────────────────────────────────
void Pet::receiveHappinessBoost(int amount) {
    stats_.happiness = clamp(stats_.happiness + static_cast<float>(amount), 0.0f, 100.0f);
    stats_.energy    = clamp(stats_.energy    - static_cast<float>(amount) * 0.3f, 0.0f, 100.0f);
}

// ── rest ──────────────────────────────────────────────────────────────────────
void Pet::rest() {
    stats_.energy    = clamp(stats_.energy    + 40.0f, 0.0f, 100.0f);
    stats_.happiness = clamp(stats_.happiness + 5.0f,  0.0f, 100.0f);
    // Resting costs no hunger but takes time — apply a moderate decay tick
    applyDecayForDuration(20.0f);
}

// ── checkAndEvolve ────────────────────────────────────────────────────────────
// Evolution thresholds (interaction counts). Health must be ≥ 50 to evolve.
bool Pet::checkAndEvolve() {
    if (stats_.health < 50.0f) return false;

    int threshold = nextEvolutionAt();
    if (threshold < 0) return false; // already Adult
    if (interactionCount_ < threshold) return false;

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
    switch (stage_) {
        case E::Baby:  return 15;
        case E::Child: return 40;
        case E::Teen:  return 80;
        case E::Adult: return -1;
    }
    return -1;
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

// ── getCurrentMood ────────────────────────────────────────────────────────────
Mood Pet::getCurrentMood() const {
    if (isSick_)                         return Mood::Sick;
    if (stats_.energy < 15.0f)           return Mood::Sleeping;
    float avg = (stats_.hunger + stats_.happiness + stats_.health) / 3.0f;
    if (avg >= 70.0f)                    return Mood::Happy;
    if (avg >= 40.0f)                    return Mood::Neutral;
    return Mood::Sad;
}

// ── isAlive ───────────────────────────────────────────────────────────────────
bool Pet::isAlive() const {
    return stats_.health > 0.0f;
}

// ── triggerRandomEvent ────────────────────────────────────────────────────────
// 8% chance each call. Events are a mix of positive and negative.
std::string Pet::triggerRandomEvent() {
    if ((std::rand() % 100) >= 8) return "";

    int roll = std::rand() % 8;
    switch (roll) {
        case 0:
            stats_.hunger = clamp(stats_.hunger + 20.0f, 0.0f, 100.0f);
            return Color::BRIGHT_GREEN + std::string("EVENT: ") + name_ +
                   " found a mystery snack! (+20 Hunger)" + Color::RESET;
        case 1:
            stats_.happiness = clamp(stats_.happiness + 15.0f, 0.0f, 100.0f);
            return Color::BRIGHT_YELLOW + std::string("EVENT: It's a sunny day! ") + name_ +
                   " is cheerful! (+15 Happiness)" + Color::RESET;
        case 2:
            if (!isSick_) {
                isSick_ = true;
                return Color::BRIGHT_RED + std::string("EVENT: Oh no! ") + name_ +
                       " caught a cold! (Health drains faster)" + Color::RESET;
            }
            // If already sick, recover instead
            isSick_ = false;
            stats_.health = clamp(stats_.health + 10.0f, 0.0f, 100.0f);
            return Color::BRIGHT_GREEN + std::string("EVENT: ") + name_ +
                   " recovered from illness! (+10 Health)" + Color::RESET;
        case 3:
            stats_.energy = clamp(stats_.energy - 20.0f, 0.0f, 100.0f);
            return Color::BLUE + std::string("EVENT: ") + name_ +
                   " had a nightmare! (-20 Energy)" + Color::RESET;
        case 4:
            stats_.happiness = clamp(stats_.happiness + 20.0f, 0.0f, 100.0f);
            return Color::BRIGHT_CYAN + std::string("EVENT: ") + name_ +
                   " made a new friend! (+20 Happiness)" + Color::RESET;
        case 5:
            stats_.happiness = clamp(stats_.happiness - 15.0f, 0.0f, 100.0f);
            return Color::YELLOW + std::string("EVENT: ") + name_ +
                   " broke a favourite toy. (-15 Happiness)" + Color::RESET;
        case 6:
            stats_.health = clamp(stats_.health + 15.0f, 0.0f, 100.0f);
            return Color::BRIGHT_GREEN + std::string("EVENT: ") + name_ +
                   " drank a health potion! (+15 Health)" + Color::RESET;
        case 7:
            stats_.hunger    = clamp(stats_.hunger    + 10.0f, 0.0f, 100.0f);
            stats_.happiness = clamp(stats_.happiness + 10.0f, 0.0f, 100.0f);
            stats_.energy    = clamp(stats_.energy    + 10.0f, 0.0f, 100.0f);
            return Color::BRIGHT_MAGENTA + std::string("EVENT: A shooting star! ") + name_ +
                   " feels blessed! (+10 all stats)" + Color::RESET;
        default:
            return "";
    }
}

// ── serialize ─────────────────────────────────────────────────────────────────
// Produces a flat JSON object. No dependencies required.
std::string Pet::serialize() const {
    std::ostringstream ss;
    ss << "{\n"
       << "  \"name\": \""         << name_             << "\",\n"
       << "  \"hunger\": "         << stats_.hunger     << ",\n"
       << "  \"happiness\": "      << stats_.happiness  << ",\n"
       << "  \"energy\": "         << stats_.energy     << ",\n"
       << "  \"health\": "         << stats_.health     << ",\n"
       << "  \"stage\": "          << static_cast<int>(stage_) << ",\n"
       << "  \"interactionCount\": "<< interactionCount_ << ",\n"
       << "  \"isSick\": "         << (isSick_ ? "true" : "false") << ",\n"
       << "  \"lastSaved\": "      << static_cast<long long>(lastSaved_) << "\n"
       << "}\n";
    return ss.str();
}

// ── deserialize ───────────────────────────────────────────────────────────────
// Parses the flat JSON written by serialize(). Uses the local jsonGet helper.
Pet Pet::deserialize(const std::string& json) {
    std::string name = jsonGet(json, "name");
    if (name.empty()) throw std::runtime_error("Invalid save file: missing name");

    Pet pet(name);
    try {
        pet.stats_.hunger       = std::stof(jsonGet(json, "hunger"));
        pet.stats_.happiness    = std::stof(jsonGet(json, "happiness"));
        pet.stats_.energy       = std::stof(jsonGet(json, "energy"));
        pet.stats_.health       = std::stof(jsonGet(json, "health"));
        pet.stage_              = static_cast<EvolutionStage>(std::stoi(jsonGet(json, "stage")));
        pet.interactionCount_   = std::stoi(jsonGet(json, "interactionCount"));
        pet.isSick_             = (jsonGet(json, "isSick") == "true");
        pet.lastSaved_          = static_cast<std::time_t>(std::stoll(jsonGet(json, "lastSaved")));
    } catch (const std::exception& e) {
        throw std::runtime_error(std::string("Save file parse error: ") + e.what());
    }
    pet.clampStats();
    return pet;
}
