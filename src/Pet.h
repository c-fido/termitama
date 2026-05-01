#pragma once
#include "Display.h"   // brings in PetState enums
#include <string>
#include <ctime>

// Pull the shared enums into Pet so callers use Pet::EvolutionStage / Pet::Mood
using PetState::EvolutionStage;
using PetState::Mood;

class Pet {
public:
    // ── Nested types ──────────────────────────────────────────────────────────
    struct Stats {
        float hunger    = 80.0f;  // 100 = full,   0 = starving
        float happiness = 80.0f;  // 100 = joyful,  0 = miserable
        float energy    = 80.0f;  // 100 = rested,  0 = exhausted
        float health    = 100.0f; // 100 = perfect, 0 = dead
    };

    // Per-minute stat decay rates. The Adult stage has a 0.7x multiplier,
    // so it decays ~30% slower than earlier stages (reward for good care).
    struct DecayRates {
        float hungerPerMin    = 0.50f;
        float happinessPerMin = 0.30f;
        float energyPerMin    = 0.20f;
        // Health only decays when other stats are critically low (< 20)
        float healthPerMin    = 0.08f;
    };

    // ── Construction & naming ─────────────────────────────────────────────────
    explicit Pet(const std::string& name);

    // ── Core actions (called by GameManager) ──────────────────────────────────
    void feed(int amount = 30);
    void receiveHappinessBoost(int amount);
    void rest();

    // ── Time-decay engine ─────────────────────────────────────────────────────
    // Called on load: retroactively apply stat decay for the given offline
    // duration. Returns a human-readable summary of what happened.
    std::string applyOfflineDecay(long long minutesPassed);

    // Called after every player interaction: small real-time passive tick.
    void applyInteractionDecay();

    // ── Evolution ─────────────────────────────────────────────────────────────
    // Returns true and updates stage/name if threshold crossed.
    bool checkAndEvolve();

    // ── Random events ─────────────────────────────────────────────────────────
    // ~8% chance per interaction; returns event description or "" if none.
    std::string triggerRandomEvent();

    // ── Serialization (manual JSON, no external deps) ─────────────────────────
    std::string serialize() const;
    static Pet deserialize(const std::string& json);

    // ── Queries ───────────────────────────────────────────────────────────────
    Mood          getCurrentMood() const;
    bool          isAlive()        const;
    std::string   getStageName()   const;
    int           nextEvolutionAt()const; // interaction count for next stage

    // ── Accessors ─────────────────────────────────────────────────────────────
    const std::string&  getName()             const { return name_; }
    const Stats&        getStats()            const { return stats_; }
    EvolutionStage      getStage()            const { return stage_; }
    int                 getInteractionCount() const { return interactionCount_; }
    bool                isSick()              const { return isSick_; }
    std::time_t         getLastSaved()        const { return lastSaved_; }
    void                stampSaveTime()             { lastSaved_ = std::time(nullptr); }
    void                incrementInteractions()     { ++interactionCount_; }

private:
    std::string    name_;
    Stats          stats_;
    EvolutionStage stage_         = EvolutionStage::Baby;
    int            interactionCount_ = 0;
    bool           isSick_        = false;
    std::time_t    lastSaved_     = 0;

    DecayRates getDecayRates() const;
    void       clampStats();
    void       applyDecayForDuration(float minutes);
};
