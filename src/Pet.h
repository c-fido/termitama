#pragma once
#include "Display.h"
#include <string>
#include <ctime>
#include <sstream>

using PetState::EvolutionStage;
using PetState::Mood;

enum class FSMState { Idle = 0, Sleeping = 1, Playing = 2, Sick = 3, Dead = 4 };

struct Genome {
    float maxHunger        = 100.0f;
    float maxHappiness     = 100.0f;
    float maxEnergy        = 100.0f;
    float maxHealth        = 100.0f;
    float decayResistance  = 1.0f;
    int   evolutionVariant = 0;
};

class Pet {
public:
    struct Stats {
        float hunger    = 80.0f;
        float happiness = 80.0f;
        float energy    = 80.0f;
        float health    = 100.0f;
    };

    struct DecayRates {
        float hungerPerMin    = 0.50f;
        float happinessPerMin = 0.30f;
        float energyPerMin    = 0.20f;
        float healthPerMin    = 0.08f;
    };

    explicit Pet(const std::string& name);

    void feed(int amount = 30);
    void receiveHappinessBoost(int amount);
    void rest();

    void applyFoodEffect(int hungerAmount, int happinessBonus);
    void applyMedicine(int healthAmount, bool curesSickness);

    void addEnergy(float amount);

    std::string applyOfflineDecay(long long minutesPassed);
    void        applyInteractionDecay();

    bool        checkAndEvolve();
    int         nextEvolutionAt() const;
    std::string getStageName()    const;

    std::string triggerRandomEvent();

    void       serializeToJson(std::ostringstream& ss,
                               const std::string& prefix) const;
    static Pet deserializeFromJson(const std::string& json,
                                   const std::string& prefix);

    std::string serialize()              const;
    static Pet  deserialize(const std::string& json);

    Mood        getCurrentMood() const;
    bool        isAlive()        const { return stats_.health > 0.0f; }

    const std::string& getName()             const { return name_; }
    const Stats&       getStats()            const { return stats_; }
    EvolutionStage     getStage()            const { return stage_; }
    int                getInteractionCount() const { return interactionCount_; }
    bool               isSick()              const { return isSick_; }
    std::time_t        getLastSaved()        const { return lastSaved_; }
    FSMState           getFSMState()         const { return fsmState_; }
    const Genome&      getGenome()           const { return genome_; }

    void setFSMState(FSMState s)         { fsmState_ = s; }
    void setGenome(const Genome& g)      { genome_ = g; }
    void setSick(bool v)                 { isSick_ = v; }
    void stampSaveTime()                 { lastSaved_ = std::time(nullptr); }
    void incrementInteractions()         { ++interactionCount_; }

private:
    std::string    name_;
    Stats          stats_;
    EvolutionStage stage_            = EvolutionStage::Baby;
    int            interactionCount_ = 0;
    bool           isSick_           = false;
    std::time_t    lastSaved_        = 0;
    FSMState       fsmState_         = FSMState::Idle;
    Genome         genome_;

    DecayRates getDecayRates()                const;
    void       clampStats();
    void       applyDecayForDuration(float minutes);

    static std::string jsonGet(const std::string& json, const std::string& key);
};
