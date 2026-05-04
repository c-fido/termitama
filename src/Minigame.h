#pragma once
#include <string>
#include <cstdlib>

struct MinigameSession {
    static constexpr int MAX_GUESSES = 6;

    bool        active       = false;
    bool        finished     = false;
    int         secret       = 0;
    int         attemptsLeft = MAX_GUESSES;
    int         result       = 0;
    std::string feedback;
    std::string inputBuf;

    void start() {
        secret       = (std::rand() % 100) + 1;
        attemptsLeft = MAX_GUESSES;
        result       = 0;
        feedback     = "Guess a number between 1 and 100!";
        inputBuf.clear();
        active   = true;
        finished = false;
    }

    void submitGuess(int n) {
        if (finished) return;
        if (n < 1 || n > 100) {
            feedback = "Out of range! Enter a number from 1 to 100.";
            return;
        }
        --attemptsLeft;
        if (n == secret) {
            result   = 30;
            feedback = "Correct! Your pet is thrilled! (+30 Happiness)";
            finished = true;
        } else if (attemptsLeft <= 0) {
            result   = 10;
            feedback = "Out of guesses! The number was " +
                       std::to_string(secret) + ". (+10 Happiness)";
            finished = true;
        } else {
            std::string dir = (n < secret) ? "Too low!  " : "Too high! ";
            feedback = dir + std::to_string(attemptsLeft) + " guess(es) left.";
        }
        inputBuf.clear();
    }

    void reset() { *this = MinigameSession{}; }
};
