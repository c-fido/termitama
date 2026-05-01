#pragma once
#include "Pet.h"
#include <string>
#include <optional>

class GameManager {
public:
    explicit GameManager(const std::string& saveFilePath);

    // Blocking main loop — returns when the player quits or the pet dies.
    void run();

private:
    Pet         pet_;
    std::string saveFilePath_;
    bool        running_ = true;

    // ── File I/O ─────────────────────────────────────────────────────────────
    bool        loadGame();
    void        saveGame();

    // ── Time handling ────────────────────────────────────────────────────────
    // Returns minutes elapsed since pet_.getLastSaved() using std::chrono.
    long long   minutesSinceLastSave() const;

    // ── UI helpers ───────────────────────────────────────────────────────────
    void        printStatus() const;
    void        printMenu()   const;

    // ── Action handlers ──────────────────────────────────────────────────────
    void        handleFeed();
    void        handlePlay();
    void        handleSleep();
    void        handleQuit();

    // ── Post-action bookkeeping ──────────────────────────────────────────────
    // Applies passive decay, increments interaction count, checks evolution,
    // and rolls for a random event. Returns any event message to display.
    std::string postActionTick();
};
