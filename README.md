# 🐾 Virtual Pet — A Terminal Tamagotchi Clone

A C++17 terminal-based virtual pet game inspired by the classic Tamagotchi. Raise, feed, play with, and evolve your digital companions.

---

## Features

- **Pet lifecycle** — pets evolve through four stages: Baby → Child → Teen → Adult
- **Stat management** — track Hunger, Happiness, Energy, and Health in real time
- **Offline decay** — stats degrade while the game is closed; check in regularly
- **Shop & inventory** — earn coins by playing minigames and spend them on food, medicine, and toys
- **Pet Box** — own up to 6 pets simultaneously and switch between them freely
- **Genetics & breeding** — breed two Adult pets to produce offspring with inherited (and mutated) traits
- **Persistence** — game auto-saves after every action; pick up exactly where you left off

---

## Requirements

- **C++17** compatible compiler (GCC 7+, Clang 5+, or MSVC 2017+)
- **CMake 3.14+**
- A terminal with ANSI color support (macOS Terminal, Linux terminal, Windows Terminal)

[FTXUI](https://github.com/ArthurSonzogni/FTXUI) v5 is fetched automatically by CMake at configure time — no manual install needed.

---

## Building & Running

```bash
# Clone the repository
git clone https://github.com/cfido/termitama.git
cd termitama

# Create a build directory and compile
mkdir build
cd build
cmake ..
cmake --build .

# Run the game
./virtual_pet
```

The save file (`savegame_v2.json`) is created automatically in the directory where you run the executable.

---

## Interface

The game uses a full-terminal TUI built with FTXUI. The main screen shows your pet's ASCII art, live stat bars, FSM state, coin count, and an event log. Overlaid modals open for the Shop, Inventory, Pet Box, and Minigame.

### Navigation

| Key | Action |
|---|---|
| Arrow keys | Navigate lists in Shop, Inventory, and Pet Box |
| Enter | Confirm selection or submit a text field |
| Escape | Close the current modal and return to the main view |

Buttons are clickable with the mouse if your terminal supports it.

---

## How to Play

On first launch you'll be prompted to name your pet. After that, the main screen has the following buttons when your pet is awake:

**Feed** — Restore 30 hunger (free, no items required).

**Play** — Launch a number-guessing minigame. You earn coins win or lose:
- Win: **+50 coins** and a happiness boost
- Lose: **+15 coins** and a smaller happiness boost

**Rest** — Give your pet a break: +40 energy, +5 happiness.

**Use Item** — Opens the Inventory modal; select an item and press Use.

**Shop** — Opens the Shop modal; browse with arrow keys and press Buy.

**Pet Box** — Manage all your pets, adopt new ones, or breed Adults together.

**Quit** — Saves and exits.

When your pet is sleeping (energy too low), only **Wait** and **Quit** are available. Wait passes 15 game-minutes and restores a small amount of energy.

---

## Stats

Your pet has four core stats, each ranging from 0 to their genetic maximum:

| Stat | What happens when it's low |
|---|---|
| **Hunger** | Pet becomes unhappy and eventually loses health |
| **Happiness** | Pet's mood deteriorates; affects overall wellbeing |
| **Energy** | Pet falls asleep and is unavailable until rested |
| **Health** | Pet can become sick; at 0, the pet dies |

Stats decay over time — even while the game is closed (up to a maximum of 12 hours of offline decay).

---

## Shop

Coins are earned by playing minigames. You start with **100 coins**.

| Item | Cost | Effect |
|---|---|---|
| Kibble | 2 | +20 hunger, +4 happiness |
| Meal | 8 | +45 hunger, +9 happiness |
| Feast | 20 | +90 hunger, +18 happiness |
| Remedy | 25 | +10 health (does not cure sickness) |
| Cure-All | 60 | +20 health and cures sickness |
| Toy Ball | 35 | +15 happiness |
| Royal Crown | 75 | +25 happiness |

Your inventory and coins are shared across all pets in your Pet Box.

---

## Evolution

Pets evolve by accumulating interactions. Each action you take counts as an interaction. Evolution also requires your pet's health to be **≥ 50** at the time of the check.

| Stage | Normal | Quick Variant | Robust Variant |
|---|---|---|---|
| Baby → Child | 15 interactions | 10 | 20 |
| Child → Teen | 40 interactions | 25 | 55 |
| Teen → Adult | 80 interactions | 50 | 110 |

The evolution variant is a genetic trait — some pets naturally develop faster or slower, and this can be inherited by offspring.

---

## Pet Box

You can own up to **6 pets** at once. Only one pet is "Active" at a time — that's the one you interact with in the main screen.

From the Pet Box modal you can:

- **Adopt** a new Baby pet (if you have fewer than 6)
- **Rename** your active pet
- **Switch** your active pet to any living pet in the box
- **Breed** two Adult-stage pets to create an offspring

If your active pet dies, the game automatically switches to another living pet in your box.

---

## Genetics & Breeding

Breeding two Adult pets produces a Baby with a genome inherited from both parents. Each stat cap (max hunger, happiness, energy, health) and the decay resistance are blended from the parents' values with a small random mutation (±10%). The evolution variant (Normal / Quick / Robust) is randomly inherited from one of the two parents.

Selective breeding over multiple generations lets you raise pets with higher stat caps and better decay resistance — pets that stay healthy longer and need less frequent attention.

---

## Saving

The game saves automatically after every action. On the next launch, offline stat decay is calculated based on how long you were away (capped at 12 hours), so your pet won't be in perfect shape after a long absence — but it won't die instantly either.

Save file: `savegame_v2.json` (created next to the executable).

---

## Tips

- **Check in regularly.** Stat decay is continuous. A pet left alone for hours will be hungry and unhappy.
- **Keep health above 50.** Your pet can't evolve while its health is low, and it can get sick if health drops too far.
- **Play minigames often.** It's the only source of income, and coins are how you afford Cure-Alls when things go wrong.
- **Breed strategically.** Pets with high stat caps and good decay resistance make for much easier care in later generations.
- **Don't neglect sleeping pets.** When your active pet is asleep, use Wait to pass time and restore its energy rather than just quitting.
