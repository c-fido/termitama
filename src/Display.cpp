#include "Display.h"
#include <iostream>
#include <sstream>
#include <iomanip>
#include <cmath>



namespace {

const char* BABY_HAPPY[] = {
    "    .~~~.    ",
    "   (o ^ o)   ",
    "   (  v  )   ",
    "    `---'    ",
    "   /|   |\\  ",
    nullptr
};
const char* BABY_NEUTRAL[] = {
    "    .~~~.    ",
    "   (o - o)   ",
    "   ( ___ )   ",
    "    `---'    ",
    "   /|   |\\  ",
    nullptr
};
const char* BABY_SAD[] = {
    "    .~~~.    ",
    "   (T _ T)   ",
    "   ( ___ )   ",
    "    `---'    ",
    "   /|   |\\  ",
    nullptr
};
const char* BABY_SLEEPING[] = {
    "    .~~~.    ",
    "   (- w -)   ",
    "   ( ___ )   ",
    "    `---'    ",
    "  z Z z Z    ",
    nullptr
};
const char* BABY_SICK[] = {
    "    .~~~.    ",
    "   (x _ x)   ",
    "   ( ___ )   ",
    "    `---'    ",
    "   ~ ~ ~     ",
    nullptr
};

const char* CHILD_HAPPY[] = {
    "   .(~~~).   ",
    "  (^ v ^ )   ",
    "  (       )  ",
    "   `-----'   ",
    "  /|     |\\  ",
    " d |     | b ",
    nullptr
};
const char* CHILD_NEUTRAL[] = {
    "   .(~~~).   ",
    "  (- _ - )   ",
    "  (       )  ",
    "   `-----'   ",
    "  /|     |\\  ",
    " d |     | b ",
    nullptr
};
const char* CHILD_SAD[] = {
    "   .(~~~).   ",
    "  (T . T )   ",
    "  (  ___  )  ",
    "   `-----'   ",
    "  /|     |\\  ",
    " d |     | b ",
    nullptr
};
const char* CHILD_SLEEPING[] = {
    "   .(~~~).   ",
    "  (- _ - )   ",
    "  (       )  ",
    "   `-----'   ",
    "  /|     |\\  ",
    "  z  Z  z    ",
    nullptr
};
const char* CHILD_SICK[] = {
    "   .(~~~).   ",
    "  (x . x )   ",
    "  (  ___  )  ",
    "   `-----'   ",
    "   ~ ~ ~     ",
    " d |     | b ",
    nullptr
};

const char* TEEN_HAPPY[] = {
    "  /\\ /\\ /\\   ",
    " ( ^  w  ^ )  ",
    " (          ) ",
    "  `--------'  ",
    "   /|    |\\  ",
    "  / |    | \\  ",
    " d  |    |  b ",
    nullptr
};
const char* TEEN_NEUTRAL[] = {
    "  /\\ /\\ /\\   ",
    " ( -  _  - )  ",
    " (          ) ",
    "  `--------'  ",
    "   /|    |\\  ",
    "  / |    | \\  ",
    " d  |    |  b ",
    nullptr
};
const char* TEEN_SAD[] = {
    "  /\\ /\\ /\\   ",
    " ( T  _  T )  ",
    " (  ______  ) ",
    "  `--------'  ",
    "   /|    |\\  ",
    "  / |    | \\  ",
    " d  |    |  b ",
    nullptr
};
const char* TEEN_SLEEPING[] = {
    "  /\\ /\\ /\\   ",
    " ( -  _  - )  ",
    " (          ) ",
    "  `--------'  ",
    "   /|    |\\  ",
    "  z   Z   z   ",
    nullptr
};
const char* TEEN_SICK[] = {
    "  /\\ /\\ /\\   ",
    " ( x  _  x )  ",
    " (  ______  ) ",
    "  `--------'  ",
    "    ~ ~ ~     ",
    "  / |    | \\  ",
    nullptr
};

const char* ADULT_HAPPY[] = {
    " *  /^^^^^\\  * ",
    "   ( O . O )   ",
    "   (  \\_/  )   ",
    "    `-----'    ",
    "   //|   |\\\\ ",
    "  // |   | \\\\  ",
    " dd  |   |  bb ",
    nullptr
};
const char* ADULT_NEUTRAL[] = {
    "    /^^^^^\\    ",
    "   ( O _ O )   ",
    "   (  ===  )   ",
    "    `-----'    ",
    "   //|   |\\\\ ",
    "  // |   | \\\\  ",
    " dd  |   |  bb ",
    nullptr
};
const char* ADULT_SAD[] = {
    "    /^^^^^\\    ",
    "   ( T _ T )   ",
    "   (  ___  )   ",
    "    `-----'    ",
    "   //|   |\\\\ ",
    "  // |   | \\\\  ",
    " dd  |   |  bb ",
    nullptr
};
const char* ADULT_SLEEPING[] = {
    "    /^^^^^\\    ",
    "   ( - _ - )   ",
    "   (  ===  )   ",
    "    `-----'    ",
    "   //|   |\\\\ ",
    "  z  Z  z  Z   ",
    nullptr
};
const char* ADULT_SICK[] = {
    "    /^^^^^\\    ",
    "   ( x _ x )   ",
    "   (  ___  )   ",
    "    `-----'    ",
    "    ~ ~ ~ ~    ",
    "  // |   | \\\\  ",
    nullptr
};

const char** selectArt(PetState::EvolutionStage stage, PetState::Mood mood) {
    using S = PetState::EvolutionStage;
    using M = PetState::Mood;
    switch (stage) {
        case S::Baby:
            switch (mood) {
                case M::Happy:    return BABY_HAPPY;
                case M::Neutral:  return BABY_NEUTRAL;
                case M::Sad:      return BABY_SAD;
                case M::Sleeping: return BABY_SLEEPING;
                case M::Sick:     return BABY_SICK;
            }
            break;
        case S::Child:
            switch (mood) {
                case M::Happy:    return CHILD_HAPPY;
                case M::Neutral:  return CHILD_NEUTRAL;
                case M::Sad:      return CHILD_SAD;
                case M::Sleeping: return CHILD_SLEEPING;
                case M::Sick:     return CHILD_SICK;
            }
            break;
        case S::Teen:
            switch (mood) {
                case M::Happy:    return TEEN_HAPPY;
                case M::Neutral:  return TEEN_NEUTRAL;
                case M::Sad:      return TEEN_SAD;
                case M::Sleeping: return TEEN_SLEEPING;
                case M::Sick:     return TEEN_SICK;
            }
            break;
        case S::Adult:
            switch (mood) {
                case M::Happy:    return ADULT_HAPPY;
                case M::Neutral:  return ADULT_NEUTRAL;
                case M::Sad:      return ADULT_SAD;
                case M::Sleeping: return ADULT_SLEEPING;
                case M::Sick:     return ADULT_SICK;
            }
            break;
    }
    return BABY_NEUTRAL;
}

const char* artColor(PetState::Mood mood, PetState::EvolutionStage stage) {
    using M = PetState::Mood;
    using S = PetState::EvolutionStage;
    if (mood == M::Sick)     return Color::BRIGHT_RED;
    if (mood == M::Sleeping) return Color::DIM;
    if (mood == M::Sad)      return Color::BLUE;
    if (stage == S::Adult)   return Color::BRIGHT_MAGENTA;
    if (stage == S::Teen)    return Color::BRIGHT_CYAN;
    if (stage == S::Child)   return Color::BRIGHT_GREEN;
    return Color::BRIGHT_YELLOW;
}

}


std::string Display::getAsciiArt(PetState::EvolutionStage stage, PetState::Mood mood) {
    const char** lines = selectArt(stage, mood);
    const char* col    = artColor(mood, stage);
    std::string out;
    for (int i = 0; lines[i] != nullptr; ++i) {
        out += col;
        out += lines[i];
        out += Color::RESET;
        out += '\n';
    }
    return out;
}

std::string Display::statBar(float value, float max, int width) {
    float ratio    = std::max(0.0f, std::min(value / max, 1.0f));
    int filled     = static_cast<int>(std::round(ratio * width));
    int empty      = width - filled;

    const char* barColor =
        (ratio > 0.60f) ? Color::BRIGHT_GREEN :
        (ratio > 0.30f) ? Color::BRIGHT_YELLOW :
                          Color::BRIGHT_RED;

    std::ostringstream ss;
    ss << "[" << barColor;
    for (int i = 0; i < filled; ++i) ss << "\xE2\x96\x88"; 
    ss << Color::DIM;
    for (int i = 0; i < empty;  ++i) ss << "\xE2\x96\x91"; 
    ss << Color::RESET << "]";
    return ss.str();
}


std::string Display::statLine(const std::string& label, float value, float max) {
    const char* valColor =
        (value / max > 0.60f) ? Color::BRIGHT_GREEN :
        (value / max > 0.30f) ? Color::BRIGHT_YELLOW :
                                 Color::BRIGHT_RED;

    std::ostringstream ss;
    ss << Color::BOLD << std::setw(12) << std::left << label << Color::RESET
       << " " << statBar(value, max)
       << " " << valColor
       << std::fixed << std::setprecision(0) << value << "/" << max
       << Color::RESET;
    return ss.str();
}

void Display::clearScreen() {

    std::cout << "\033[2J\033[H";
}

void Display::printBanner(const std::string& text, const char* color) {
    int len = static_cast<int>(text.size()) + 4;
    std::string border(len, '=');
    std::cout << color << Color::BOLD
              << border << "\n"
              << "| " << text << " |\n"
              << border << "\n"
              << Color::RESET;
}

void Display::printDivider(int width) {
    std::cout << Color::DIM << std::string(width, '-') << Color::RESET << "\n";
}
