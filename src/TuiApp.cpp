#include "TuiApp.h"
#include "Display.h"
#include "Shop.h"

#include <sstream>
#include <iomanip>
#include <algorithm>
#include <chrono>
#include <thread>
#include <cmath>

#include "ftxui/component/component.hpp"
#include "ftxui/component/event.hpp"
#include "ftxui/component/screen_interactive.hpp"
#include "ftxui/dom/elements.hpp"

using namespace ftxui;

static const std::vector<std::string> SHOP_ORDER = {
    "kibble", "meal", "feast", "remedy", "cureall", "toyball", "crown"
};

TuiApp::TuiApp(const std::string& saveFile, const std::string& legacySaveFile)
    : gm_(saveFile, legacySaveFile)
{}

TuiApp::~TuiApp() {
    timerRunning_.store(false);
    if (timerThread_.joinable()) timerThread_.join();
}

std::string TuiApp::stripAnsi(const std::string& s) {
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

void TuiApp::addLog(const std::string& raw) {
    if (raw.empty()) return;
    std::string msg = stripAnsi(raw);
    std::istringstream iss(msg);
    std::string line;
    while (std::getline(iss, line)) {
        auto first = line.find_first_not_of(" \t");
        if (first != std::string::npos) line = line.substr(first);
        if (!line.empty()) {
            log_.push_back(line);
            if (log_.size() > 5) log_.erase(log_.begin());
        }
    }
}

// Explicit ftxui::Color to avoid ambiguity with Display.h's Color namespace.
static Element makeStatBar(const std::string& label, float value, float max) {
    float ratio = (max > 0.0f) ? std::max(0.0f, std::min(value / max, 1.0f)) : 0.0f;
    ftxui::Color barCol = (ratio > 0.60f) ? ftxui::Color::Green :
                          (ratio > 0.30f) ? ftxui::Color::Yellow :
                                            ftxui::Color::Red;
    std::ostringstream val;
    val << static_cast<int>(value) << "/" << static_cast<int>(max);
    return hbox({
        text(label) | bold | size(WIDTH, EQUAL, 11),
        gauge(ratio)  | color(barCol) | size(WIDTH, EQUAL, 14),
        text(" " + val.str()) | color(barCol) | size(WIDTH, EQUAL, 9),
    });
}

static ftxui::Color petColor(PetState::Mood mood, PetState::EvolutionStage stage) {
    using M = PetState::Mood;
    using S = PetState::EvolutionStage;
    if (mood == M::Sick)     return ftxui::Color::RedLight;
    if (mood == M::Sleeping) return ftxui::Color::GrayDark;
    if (mood == M::Sad)      return ftxui::Color::BlueLight;
    switch (stage) {
        case S::Adult: return ftxui::Color::MagentaLight;
        case S::Teen:  return ftxui::Color::CyanLight;
        case S::Child: return ftxui::Color::GreenLight;
        default:       return ftxui::Color::YellowLight;
    }
}

void TuiApp::run() {
    std::string loadMsg;
    if (gm_.load(loadMsg)) {
        addLog(loadMsg);
    } else {
        activeView_ = 5;
    }

    auto screen = ScreenInteractive::Fullscreen();

    InputOption ngOpt;
    ngOpt.multiline = false;
    ngOpt.on_enter  = [&] {
        gm_.newGame(nameInput_);
        addLog("Welcome, " + (nameInput_.empty() ? std::string("Pixel") : nameInput_) + "!");
        nameInput_.clear();
        activeView_ = 0;
    };
    auto ngInput  = Input(&nameInput_, "e.g. Pixel", ngOpt);
    auto ngStart  = Button(" Start! ", [&] {
        gm_.newGame(nameInput_);
        addLog("Welcome, " + (nameInput_.empty() ? std::string("Pixel") : nameInput_) + "!");
        nameInput_.clear();
        activeView_ = 0;
    });
    auto tab5 = Container::Vertical({ngInput, ngStart});

    std::vector<std::string> shopLabels;
    for (const auto& id : SHOP_ORDER) {
        auto item = Shop::getItem(id);
        if (item) shopLabels.push_back(item->getName());
        else      shopLabels.push_back(id);
    }
    auto shopMenu = Menu(&shopLabels, &shopSel_);
    auto shopBuy  = Button(" Buy ", [&] {
        if (shopSel_ >= 0 && shopSel_ < static_cast<int>(SHOP_ORDER.size()))
            addLog(gm_.actionPurchase(SHOP_ORDER[static_cast<size_t>(shopSel_)]));
    });
    auto shopBack = Button(" Back ", [&] { activeView_ = 0; });
    auto tab1 = Container::Vertical({
        shopMenu,
        Container::Horizontal({shopBuy, shopBack}),
    });

    auto invUse  = Button(" Use ", [&] {
        addLog(gm_.actionUseItem(invSel_));
        // keep selection in bounds after item consumed
        int n = static_cast<int>(gm_.inventory().getAllItems().size());
        if (invSel_ >= n) invSel_ = std::max(0, n - 1);
    });
    auto invBack = Button(" Back ", [&] { activeView_ = 0; });
    auto tab2 = Container::Horizontal({invUse, invBack});

    auto pbAdopt  = Button(" Adopt ",  [&] { pbMode_ = PBMode::Adopt;  pbInput_.clear(); });
    auto pbRename = Button(" Rename ", [&] { pbMode_ = PBMode::Rename; pbInput_.clear(); });
    auto pbSwitch = Button(" Switch ", [&] {
        addLog(gm_.actionSwitchPet(pbSel_));
        activeView_ = 0;
    });
    auto pbBreed  = Button(" Breed ",  [&] {
        pbMode_ = PBMode::BreedA; pbInput_.clear();
        breedIdxA_ = breedIdxB_ = -1;
    });
    auto pbBack2  = Button(" Back ",   [&] { pbMode_ = PBMode::List; pbInput_.clear(); activeView_ = 0; });

    InputOption pbOpt;
    pbOpt.multiline = false;
    pbOpt.on_enter  = [&] {
        switch (pbMode_) {
            case PBMode::Adopt:
                addLog(gm_.actionAdoptPet(pbInput_)); pbMode_ = PBMode::List; pbInput_.clear(); break;
            case PBMode::Rename:
                addLog(gm_.actionRenamePet(pbInput_)); pbMode_ = PBMode::List; pbInput_.clear(); break;
            case PBMode::BreedName:
                addLog(gm_.actionBreedPets(breedIdxA_, breedIdxB_, pbInput_));
                pbMode_ = PBMode::List; pbInput_.clear(); breedIdxA_ = breedIdxB_ = -1; break;
            default: break;
        }
    };
    auto pbInput  = Input(&pbInput_, "Name...", pbOpt);

    auto pbConfirm = Button(" OK ", [&] {
        switch (pbMode_) {
            case PBMode::Adopt:
                addLog(gm_.actionAdoptPet(pbInput_)); pbMode_ = PBMode::List; pbInput_.clear(); break;
            case PBMode::Rename:
                addLog(gm_.actionRenamePet(pbInput_)); pbMode_ = PBMode::List; pbInput_.clear(); break;
            case PBMode::BreedName:
                addLog(gm_.actionBreedPets(breedIdxA_, breedIdxB_, pbInput_));
                pbMode_ = PBMode::List; pbInput_.clear(); breedIdxA_ = breedIdxB_ = -1; break;
            default: break;
        }
    });
    auto pbCancel  = Button(" Cancel ", [&] { pbMode_ = PBMode::List; pbInput_.clear(); });
    auto pbSelA    = Button(" Set as Parent A ", [&] {
        if (pbMode_ == PBMode::BreedA) { breedIdxA_ = pbSel_; pbMode_ = PBMode::BreedB; }
    });
    auto pbSelB    = Button(" Set as Parent B ", [&] {
        if (pbMode_ == PBMode::BreedB) { breedIdxB_ = pbSel_; pbMode_ = PBMode::BreedName; pbInput_.clear(); }
    });

    auto showPbInput = [&] {
        return pbMode_ == PBMode::Adopt || pbMode_ == PBMode::Rename || pbMode_ == PBMode::BreedName;
    };
    auto showPbBreed = [&] {
        return pbMode_ == PBMode::BreedA || pbMode_ == PBMode::BreedB;
    };

    auto pbInputSection = Container::Horizontal({pbInput, pbConfirm, pbCancel});
    auto pbBreedSection = Container::Horizontal({pbSelA, pbSelB});

    auto tab3 = Container::Vertical({
        Container::Horizontal({pbAdopt, pbRename, pbSwitch, pbBreed, pbBack2}),
        Maybe(pbInputSection, showPbInput),
        Maybe(pbBreedSection, showPbBreed),
    });

    InputOption mgOpt;
    mgOpt.multiline = false;
    mgOpt.on_enter  = [&] {
        if (minigame_.finished) {
            addLog(gm_.actionPlay(minigame_.result));
            activeView_ = 0;
            minigame_.reset();
        } else {
            try {
                int n = std::stoi(minigame_.inputBuf);
                minigame_.submitGuess(n);
            } catch (...) {
                minigame_.feedback = "Please enter a valid number!";
                minigame_.inputBuf.clear();
            }
        }
    };
    auto mgInput    = Input(&minigame_.inputBuf, "1-100", mgOpt);
    auto mgGuess    = Button(" Guess ", [&] {
        if (minigame_.finished) return;
        try {
            int n = std::stoi(minigame_.inputBuf);
            minigame_.submitGuess(n);
        } catch (...) {
            minigame_.feedback = "Please enter a valid number!";
            minigame_.inputBuf.clear();
        }
    });
    auto mgContinue = Button(" Continue ", [&] {
        if (!minigame_.finished) return;
        addLog(gm_.actionPlay(minigame_.result));
        activeView_ = 0;
        minigame_.reset();
    });

    auto showMgGuess    = [&] { return !minigame_.finished; };
    auto showMgContinue = [&] { return  minigame_.finished; };

    auto tab4 = Container::Vertical({
        Maybe(Container::Horizontal({mgInput, mgGuess}), showMgGuess),
        Maybe(mgContinue, showMgContinue),
    });

    auto btnFeed = Button(" Feed ", [&] {
        if (gm_.canInteract()) addLog(gm_.actionFeed());
    });
    auto btnPlay = Button(" Play ", [&] {
        if (!gm_.canInteract()) return;
        minigame_.start();
        activeView_ = 4;
    });
    auto btnRest = Button(" Rest ", [&] {
        if (gm_.canInteract()) addLog(gm_.actionRest());
    });
    auto btnItem = Button(" Use Item ", [&] {
        if (gm_.canInteract()) { invSel_ = 0; activeView_ = 2; }
    });
    auto btnShop = Button(" Shop ", [&] { activeView_ = 1; });
    auto btnBox  = Button(" Pet Box ", [&] {
        pbMode_ = PBMode::List; pbSel_ = 0; activeView_ = 3;
    });
    auto btnWait = Button(" Wait ", [&] {
        addLog(gm_.actionWait());
    });
    auto btnQuit = Button(" Quit ", [&] {
        gm_.actionQuit();
        screen.ExitLoopClosure()();
    });

    auto mainButtons  = Container::Horizontal({btnFeed, btnPlay, btnRest, btnItem, btnShop, btnBox});
    auto sleepButtons = Container::Horizontal({btnWait});
    auto tab0 = Container::Vertical({
        Maybe(mainButtons,  [&] { return  gm_.canInteract() && gm_.petCount() > 0; }),
        Maybe(sleepButtons, [&] { return !gm_.canInteract() && gm_.petCount() > 0; }),
        btnQuit,
    });

    auto rootTabs = Container::Tab({
        tab0,
        tab1,
        tab2,
        tab3,
        tab4,
        tab5,
    }, &activeView_);

    auto renderer = Renderer(rootTabs, [&]() -> Element {

        if (activeView_ == 5 || gm_.petCount() == 0) {
            return vbox({
                text("  VIRTUAL PET  v2.0  ") | bold | color(ftxui::Color::CyanLight) | hcenter,
                separator(),
                vbox({
                    text("No save found — welcome to Virtual Pet!") | hcenter,
                    text("") ,
                    hbox({text("Name your pet: "), ngInput->Render()}) | hcenter,
                    ngStart->Render() | hcenter,
                }) | center,
            }) | border;
        }

        const auto& pet    = gm_.getActivePet();
        const auto& stats  = pet.getStats();
        const auto& genome = pet.getGenome();
        ftxui::Color artCol = petColor(pet.getCurrentMood(), pet.getStage());

        auto artLines = Display::getAsciiArtLines(pet.getStage(), pet.getCurrentMood());
        std::vector<Element> artElems;
        artElems.push_back(filler());
        for (const auto& ln : artLines)
            artElems.push_back(text(ln) | color(artCol));
        artElems.push_back(filler());
        auto asciiPanel = vbox(artElems) | center | size(WIDTH, EQUAL, 20);

        std::string evoStr;
        int nextEvo = pet.nextEvolutionAt();
        if (nextEvo > 0) {
            int v = genome.evolutionVariant;
            std::string lineage = (v == 1) ? "Quick" : (v == 2) ? "Robust" : "Normal";
            evoStr = "Evo: " + std::to_string(pet.getInteractionCount()) +
                     "/" + std::to_string(nextEvo) + "  [" + lineage + " lineage]";
        } else {
            evoStr = pet.getName() + " has reached full evolution!";
        }

        std::vector<Element> statElems = {
            makeStatBar("Hunger",    stats.hunger,    genome.maxHunger),
            makeStatBar("Happiness", stats.happiness, genome.maxHappiness),
            makeStatBar("Energy",    stats.energy,    genome.maxEnergy),
            makeStatBar("Health",    stats.health,    genome.maxHealth),
            separator(),
            hbox({
                text("State: ") | bold,
                text(gm_.fsmStateName()) | color(ftxui::Color::CyanLight),
                text("  "),
                text(gm_.fsmStateDesc()) | dim,
            }),
            text(evoStr) | dim,
            hbox({
                text("Coins: ") | bold,
                text(std::to_string(gm_.inventory().getCurrency()))
                    | color(ftxui::Color::YellowLight),
            }),
        };
        if (pet.isSick())
            statElems.push_back(
                text("!! " + pet.getName() + " is SICK — use a Cure-All! !!") | bold | color(ftxui::Color::RedLight));
        if (!pet.isAlive())
            statElems.push_back(
                text("** " + pet.getName() + " has passed away **") | bold | color(ftxui::Color::Red));
        if (gm_.petCount() > 1)
            statElems.push_back(
                text("Pet Box: " + std::to_string(gm_.petCount()) + " pets  (active: " +
                     pet.getName() + ")") | dim);

        auto statsPanel = vbox(statElems) | flex;

        Element buttonRow;
        if (!gm_.canInteract()) {
            buttonRow = hbox({
                text(" " + pet.getName() + " is sleeping — only Wait or Quit available.  ")
                    | dim | flex,
                btnWait->Render(),
                btnQuit->Render(),
            });
        } else {
            buttonRow = hbox({
                btnFeed->Render(),
                btnPlay->Render(),
                btnRest->Render(),
                btnItem->Render(),
                btnShop->Render(),
                btnBox->Render(),
                filler(),
                btnQuit->Render(),
            });
        }

        std::vector<Element> logLines;
        for (const auto& entry : log_)
            logLines.push_back(text("  > " + entry) | dim);
        while (static_cast<int>(logLines.size()) < 3)
            logLines.push_back(text(""));

        std::string header = "  VIRTUAL PET  v2.0  |  " + pet.getName() +
                              "  [" + pet.getStageName() + "]";

        auto mainLayout = vbox({
            text(header) | bold | color(ftxui::Color::CyanLight) | hcenter,
            separator(),
            hbox({
                asciiPanel,
                separator(),
                statsPanel | flex,
            }) | flex,
            separator(),
            buttonRow,
            separator(),
            vbox(logLines) | size(HEIGHT, EQUAL, 3),
        }) | border;

        if (activeView_ == 1) {
            const auto& cat = Shop::catalog();
            std::vector<Element> shopRows;
            for (int i = 0; i < static_cast<int>(SHOP_ORDER.size()); ++i) {
                const auto& id = SHOP_ORDER[static_cast<size_t>(i)];
                auto it = cat.find(id);
                if (it == cat.end()) continue;
                const auto& item = it->second;
                int owned = gm_.inventory().getQuantity(id);
                bool sel = (i == shopSel_);
                ftxui::Color typeCol = (item->getType() == "Food")     ? ftxui::Color::GreenLight :
                                      (item->getType() == "Medicine") ? ftxui::Color::RedLight   :
                                                                        ftxui::Color::MagentaLight;
                auto row = hbox({
                    text(sel ? "> " : "  "),
                    text(item->getName()) | size(WIDTH, EQUAL, 18),
                    text(item->getType()) | color(typeCol) | size(WIDTH, EQUAL, 10),
                    text(std::to_string(item->getPrice()) + "c") | color(ftxui::Color::YellowLight) | size(WIDTH, EQUAL, 5),
                    text(" x" + std::to_string(owned)) | dim,
                });
                shopRows.push_back(sel ? (row | bold) : row);
            }
            std::string shopDesc;
            if (shopSel_ >= 0 && shopSel_ < static_cast<int>(SHOP_ORDER.size())) {
                auto it = Shop::getItem(SHOP_ORDER[static_cast<size_t>(shopSel_)]);
                if (it) shopDesc = it->getDescription();
            }
            auto shopModal = vbox({
                text("  SHOP  ") | bold | color(ftxui::Color::YellowLight) | hcenter,
                separator(),
                hbox({text(" Coins: ") | bold,
                      text(std::to_string(gm_.inventory().getCurrency())) | color(ftxui::Color::YellowLight)}),
                separator(),
                vbox(shopRows),
                separator(),
                text("  " + shopDesc) | dim,
                separator(),
                hbox({shopBuy->Render(), shopBack->Render()}) | hcenter,
            }) | border | size(WIDTH, EQUAL, 52) | clear_under | center;
            return dbox({mainLayout, shopModal});
        }

        if (activeView_ == 2) {
            auto items = gm_.inventory().getAllItems();
            std::vector<Element> invRows;
            for (int i = 0; i < static_cast<int>(items.size()); ++i) {
                const auto& [item, qty] = items[static_cast<size_t>(i)];
                bool sel = (i == invSel_);
                ftxui::Color typeCol = (item->getType() == "Food")     ? ftxui::Color::GreenLight :
                                      (item->getType() == "Medicine") ? ftxui::Color::RedLight   :
                                                                        ftxui::Color::MagentaLight;
                auto row = hbox({
                    text(sel ? "> " : "  "),
                    text(item->getName()) | size(WIDTH, EQUAL, 18),
                    text(item->getType()) | color(typeCol) | size(WIDTH, EQUAL, 10),
                    text("x" + std::to_string(qty)) | dim,
                });
                invRows.push_back(sel ? (row | bold) : row);
            }
            if (items.empty())
                invRows.push_back(text("  (empty — visit the Shop first!)") | dim);

            std::string invDesc;
            if (invSel_ >= 0 && invSel_ < static_cast<int>(items.size()))
                invDesc = items[static_cast<size_t>(invSel_)].first->getDescription();

            auto invModal = vbox({
                text("  INVENTORY  ") | bold | color(ftxui::Color::CyanLight) | hcenter,
                separator(),
                vbox(invRows),
                separator(),
                text("  " + invDesc) | dim,
                separator(),
                hbox({invUse->Render(), invBack->Render()}) | hcenter,
            }) | border | size(WIDTH, EQUAL, 52) | clear_under | center;
            return dbox({mainLayout, invModal});
        }

        if (activeView_ == 3) {
            std::vector<Element> petRows;
            for (int i = 0; i < gm_.petCount(); ++i) {
                const auto& p   = gm_.getPet(i);
                bool sel = (i == pbSel_);
                ftxui::Color sCol = (p.getStage() == EvolutionStage::Adult) ? ftxui::Color::MagentaLight :
                                   (p.getStage() == EvolutionStage::Teen)  ? ftxui::Color::CyanLight    :
                                   (p.getStage() == EvolutionStage::Child) ? ftxui::Color::GreenLight   :
                                                                              ftxui::Color::YellowLight;
                std::string active = (i == gm_.activePetIndex()) ? " <active>" : "";
                std::string dead   = !p.isAlive() ? " (deceased)" : "";
                auto row = hbox({
                    text(sel ? "> " : "  "),
                    text(p.getName())     | color(sCol) | size(WIDTH, EQUAL, 14),
                    text("[" + p.getStageName() + "]") | dim | size(WIDTH, EQUAL, 9),
                    text(active) | color(ftxui::Color::CyanLight),
                    text(dead)   | color(ftxui::Color::RedLight),
                });
                petRows.push_back(sel ? (row | bold) : row);
            }

            std::string modeHint;
            switch (pbMode_) {
                case PBMode::Adopt:     modeHint = "Name for new pet:"; break;
                case PBMode::Rename:    modeHint = "New name for " + gm_.getActivePet().getName() + ":"; break;
                case PBMode::BreedA:    modeHint = "Select Parent A with the arrow keys, then click Set A."; break;
                case PBMode::BreedB:    modeHint = "Select Parent B, then click Set B."; break;
                case PBMode::BreedName: modeHint = "Name for the offspring:"; break;
                default: break;
            }

            std::vector<Element> pbElems = {
                text("  PET BOX  ") | bold | color(ftxui::Color::CyanLight) | hcenter,
                separator(),
                vbox(petRows),
                separator(),
                hbox({pbAdopt->Render(), pbRename->Render(),
                      pbSwitch->Render(), pbBreed->Render(), pbBack2->Render()}) | hcenter,
            };
            if (!modeHint.empty()) {
                pbElems.push_back(separator());
                pbElems.push_back(text("  " + modeHint) | dim);
            }
            if (showPbInput()) {
                pbElems.push_back(
                    hbox({text(" "), pbInput->Render(), pbConfirm->Render(), pbCancel->Render()}));
            }
            if (showPbBreed()) {
                pbElems.push_back(
                    hbox({pbSelA->Render(), pbSelB->Render()}) | hcenter);
            }

            auto pbModal = vbox(pbElems) | border | size(WIDTH, EQUAL, 58) | clear_under | center;
            return dbox({mainLayout, pbModal});
        }

        if (activeView_ == 4) {
            ftxui::Color fbCol = minigame_.finished ? ftxui::Color::GreenLight : ftxui::Color::White;
            std::vector<Element> mgElems = {
                text("  NUMBER GUESSING GAME  ") | bold | color(ftxui::Color::CyanLight) | hcenter,
                separator(),
                text("Guess the secret number between 1 and 100.") | hcenter,
                text("Attempts left: " + std::to_string(minigame_.attemptsLeft)) | dim | hcenter,
                separator(),
                text(minigame_.feedback) | color(fbCol) | hcenter,
                separator(),
            };
            if (!minigame_.finished) {
                mgElems.push_back(
                    hbox({text(" Your guess: "), mgInput->Render(), mgGuess->Render()}) | hcenter);
            } else {
                mgElems.push_back(mgContinue->Render() | hcenter);
            }
            auto mgModal = vbox(mgElems) | border | size(WIDTH, EQUAL, 52) | clear_under | center;
            return dbox({mainLayout, mgModal});
        }

        return mainLayout;
    });

    auto root = CatchEvent(renderer, [&](Event event) -> bool {

        if (event == Event::Custom) {
            if (gm_.petCount() > 0 && gm_.isRunning())
                addLog(gm_.tick());
            return true;
        }

        if (event == Event::Escape) {
            if (activeView_ != 0 && activeView_ != 5) {
                activeView_ = 0;
                pbMode_     = PBMode::List;
                pbInput_.clear();
                minigame_.reset();
                return true;
            }
        }

        if (activeView_ == 1) {
            if (event == Event::ArrowUp)   { if (shopSel_ > 0) --shopSel_;  return true; }
            if (event == Event::ArrowDown) {
                if (shopSel_ < static_cast<int>(SHOP_ORDER.size()) - 1) ++shopSel_;
                return true;
            }
            if (event == Event::Return) {
                if (shopSel_ >= 0 && shopSel_ < static_cast<int>(SHOP_ORDER.size()))
                    addLog(gm_.actionPurchase(SHOP_ORDER[static_cast<size_t>(shopSel_)]));
                return true;
            }
        }

        if (activeView_ == 2) {
            int n = static_cast<int>(gm_.inventory().getAllItems().size());
            if (event == Event::ArrowUp)   { if (invSel_ > 0)     --invSel_;  return true; }
            if (event == Event::ArrowDown) { if (invSel_ < n - 1) ++invSel_;  return true; }
        }

        if (activeView_ == 3) {
            if (event == Event::ArrowUp)   { if (pbSel_ > 0)                    --pbSel_; return true; }
            if (event == Event::ArrowDown) { if (pbSel_ < gm_.petCount() - 1)   ++pbSel_; return true; }
        }

        return false;
    });

    timerRunning_.store(true);
    timerThread_ = std::thread([&] {
        int ticks = 0;
        while (timerRunning_.load()) {
            std::this_thread::sleep_for(std::chrono::milliseconds(100));
            if (++ticks >= 100) {          // 100 × 100 ms = 10 s → 1 game-minute of decay
                ticks = 0;
                if (timerRunning_.load())
                    screen.PostEvent(Event::Custom);
            }
        }
    });

    screen.Loop(root);

    timerRunning_.store(false);
    if (timerThread_.joinable()) timerThread_.join();
}
