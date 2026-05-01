#include "Minigame.h"
#include "Display.h"
#include <iostream>
#include <cstdlib>
#include <string>
#include <limits>

int Minigame::runGuessingGame() {
    const int MAX_GUESSES = 6;
    const int secret = (std::rand() % 100) + 1; // 1..100

    Display::printDivider();
    std::cout << Color::BRIGHT_CYAN << Color::BOLD
              << "  *** NUMBER GUESSING GAME ***\n" << Color::RESET
              << "  I'm thinking of a number between 1 and 100.\n"
              << "  You have " << MAX_GUESSES << " guesses. Good luck!\n\n";

    for (int attempt = 1; attempt <= MAX_GUESSES; ++attempt) {
        std::cout << Color::BOLD << "  Guess #" << attempt << "/" << MAX_GUESSES
                  << ": " << Color::RESET;

        int guess = 0;
        if (!(std::cin >> guess)) {
            // Handle non-numeric input gracefully
            std::cin.clear();
            std::cin.ignore(std::numeric_limits<std::streamsize>::max(), '\n');
            std::cout << Color::YELLOW << "  Please enter a number.\n" << Color::RESET;
            --attempt; // don't count bad input as a guess
            continue;
        }
        std::cin.ignore(std::numeric_limits<std::streamsize>::max(), '\n');

        if (guess < 1 || guess > 100) {
            std::cout << Color::YELLOW << "  Out of range! Guess between 1 and 100.\n" << Color::RESET;
            --attempt;
            continue;
        }

        if (guess == secret) {
            std::cout << Color::BRIGHT_GREEN << Color::BOLD
                      << "\n  Correct! You got it in " << attempt << " guess(es)!\n"
                      << "  Your pet is thrilled! (+30 Happiness)\n"
                      << Color::RESET;
            Display::printDivider();
            return 30;
        }

        int remaining = MAX_GUESSES - attempt;
        if (guess < secret)
            std::cout << Color::BLUE << "  Too low!  ";
        else
            std::cout << Color::RED  << "  Too high! ";

        if (remaining > 0)
            std::cout << remaining << " guess(es) left.\n" << Color::RESET;
        else
            std::cout << Color::RESET << "\n";
    }

    std::cout << Color::YELLOW
              << "\n  Out of guesses! The number was " << secret << ".\n"
              << "  Your pet appreciates the effort! (+10 Happiness)\n"
              << Color::RESET;
    Display::printDivider();
    return 10;
}
