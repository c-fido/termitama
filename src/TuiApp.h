#pragma once
#include "GameManager.h"
#include "Minigame.h"
#include <string>
#include <vector>
#include <thread>
#include <atomic>

class TuiApp {
public:
    TuiApp(const std::string& saveFile, const std::string& legacySaveFile);
    ~TuiApp();

    void run();

private:
    GameManager gm_;

    // 0=main, 1=shop, 2=inventory, 3=petbox, 4=minigame, 5=newgame
    int activeView_ = 0;

    std::vector<std::string> log_;
    void addLog(const std::string& msg);

    std::string nameInput_;

    int shopSel_ = 0;
    int invSel_ = 0;

    enum class PBMode { List, Adopt, Rename, BreedA, BreedB, BreedName };
    PBMode      pbMode_     = PBMode::List;
    int         pbSel_      = 0;
    std::string pbInput_;
    int         breedIdxA_  = -1;
    int         breedIdxB_  = -1;

    MinigameSession minigame_;

    std::thread      timerThread_;
    std::atomic<bool> timerRunning_{false};

    static std::string stripAnsi(const std::string& s);
};
