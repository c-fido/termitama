#pragma once

// A simple terminal minigame used by the Play action.
// The computer picks a secret number 1-100; the player gets 6 guesses
// with higher/lower hints after each. Returns the happiness bonus earned.
class Minigame {
public:
    // Runs the game interactively. Returns happiness points awarded:
    //   Win  → 30 pts
    //   Lose → 10 pts (consolation for trying)
    static int runGuessingGame();
};
