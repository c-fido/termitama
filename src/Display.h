#pragma once
#include <string>
#include <vector>


namespace PetState {
    enum class EvolutionStage : int { Baby = 0, Child = 1, Teen = 2, Adult = 3 };
    enum class Mood { Happy, Neutral, Sad, Sleeping, Sick };
}


namespace Color {
    constexpr const char* RESET          = "\033[0m";
    constexpr const char* BOLD           = "\033[1m";
    constexpr const char* DIM            = "\033[2m";
    constexpr const char* RED            = "\033[31m";
    constexpr const char* GREEN          = "\033[32m";
    constexpr const char* YELLOW         = "\033[33m";
    constexpr const char* BLUE           = "\033[34m";
    constexpr const char* MAGENTA        = "\033[35m";
    constexpr const char* CYAN           = "\033[36m";
    constexpr const char* WHITE          = "\033[37m";
    constexpr const char* BRIGHT_RED     = "\033[91m";
    constexpr const char* BRIGHT_GREEN   = "\033[92m";
    constexpr const char* BRIGHT_YELLOW  = "\033[93m";
    constexpr const char* BRIGHT_BLUE    = "\033[94m";
    constexpr const char* BRIGHT_MAGENTA = "\033[95m";
    constexpr const char* BRIGHT_CYAN    = "\033[96m";
    constexpr const char* BRIGHT_WHITE   = "\033[97m";
    constexpr const char* BG_BLUE        = "\033[44m";
    constexpr const char* BG_CYAN        = "\033[46m";
}


namespace Display {

    std::string getAsciiArt(PetState::EvolutionStage stage, PetState::Mood mood);

    // Returns the raw ASCII art lines without ANSI colour codes (for FTXUI rendering).
    std::vector<std::string> getAsciiArtLines(PetState::EvolutionStage stage,
                                               PetState::Mood mood);

    // Progress bar (legacy stdout helpers kept for reference)
    std::string statBar(float value, float max = 100.0f, int width = 16);
    std::string statLine(const std::string& label, float value, float max = 100.0f);

    void clearScreen();
    void printBanner(const std::string& text, const char* color = Color::BRIGHT_CYAN);
    void printDivider(int width = 50);
}
