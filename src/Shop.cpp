#include "Shop.h"
#include "Display.h"
#include <iostream>
#include <limits>
#include <iomanip>

const ItemCatalog& Shop::catalog() {
    static ItemCatalog cat = [] {
        ItemCatalog c;
        c["kibble"] = std::make_shared<FoodItem>(
            "kibble", "Basic Kibble",
            "Simple dry food. Does the job.",
            20, 20, 2);
        c["meal"] = std::make_shared<FoodItem>(
            "meal", "Tasty Meal",
            "A hearty balanced meal.",
            45, 35, 8);
        c["feast"] = std::make_shared<FoodItem>(
            "feast", "Gourmet Feast",
            "A lavish five-course dinner!",
            90, 55, 20);
        c["remedy"] = std::make_shared<MedicineItem>(
            "remedy", "Herbal Remedy",
            "Boosts health but won't cure sickness.",
            25, 10, false);
        c["cureall"] = std::make_shared<MedicineItem>(
            "cureall", "Cure-All",
            "Full treatment: restores health AND cures any illness.",
            60, 20, true);
        c["toyball"] = std::make_shared<CosmeticItem>(
            "toyball", "Toy Ball",
            "A bouncy rubber ball. Great fun!",
            35, 15);
        c["crown"] = std::make_shared<CosmeticItem>(
            "crown", "Royal Crown",
            "Your pet will feel like royalty.",
            75, 25);
        return c;
    }();
    return cat;
}

std::shared_ptr<Item> Shop::getItem(const std::string& id) {
    const auto& cat = catalog();
    auto it = cat.find(id);
    return (it != cat.end()) ? it->second : nullptr;
}

void Shop::printCatalog(const Inventory& inv) {
    const auto& cat = catalog();

    static const std::vector<std::string> ORDER = {
        "kibble", "meal", "feast", "remedy", "cureall", "toyball", "crown"
    };

    std::cout << Color::BOLD << Color::BRIGHT_YELLOW
              << "\n  #   Item              Type        Price   Own\n"
              << "  -   ----              ----        -----   ---\n"
              << Color::RESET;

    int idx = 1;
    for (const auto& id : ORDER) {
        auto it = cat.find(id);
        if (it == cat.end()) continue;
        const auto& item = it->second;
        int owned = inv.getQuantity(id);

        const char* typeColor =
            (item->getType() == "Food")      ? Color::BRIGHT_GREEN  :
            (item->getType() == "Medicine")  ? Color::BRIGHT_RED    :
                                               Color::BRIGHT_MAGENTA;

        std::cout << "  " << Color::BOLD << idx++ << Color::RESET << ")  "
                  << std::left << std::setw(18) << item->getName()
                  << typeColor << std::setw(12) << item->getType() << Color::RESET
                  << Color::BRIGHT_YELLOW << std::setw(8) << item->getPrice()
                  << Color::RESET
                  << Color::DIM << owned << Color::RESET << "\n"
                  << Color::DIM << "        " << item->getDescription() << Color::RESET << "\n";
    }
}

void Shop::run(Inventory& inv, const std::string& petName) {
    static const std::vector<std::string> ORDER = {
        "kibble", "meal", "feast", "remedy", "cureall", "toyball", "crown"
    };
    const auto& cat = catalog();

    while (true) {
        Display::clearScreen();
        Display::printBanner("  SHOP — Shopping for " + petName + "  ", Color::BRIGHT_YELLOW);
        std::cout << Color::BRIGHT_YELLOW << "\n  Coins: " << inv.getCurrency()
                  << " coins\n" << Color::RESET;

        printCatalog(inv);

        std::cout << Color::DIM << "\n  Enter item number to buy, or 0 to leave: "
                  << Color::RESET;

        std::string input;
        std::getline(std::cin, input);

        if (input == "0" || input.empty()) break;

        int choice = 0;
        try { choice = std::stoi(input); } catch (...) { continue; }

        if (choice < 1 || choice > static_cast<int>(ORDER.size())) {
            std::cout << Color::YELLOW << "  Invalid choice.\n" << Color::RESET;
            continue;
        }

        const std::string& id = ORDER[static_cast<size_t>(choice) - 1];
        auto it = cat.find(id);
        if (it == cat.end()) continue;
        const auto& item = it->second;

        if (inv.getCurrency() < item->getPrice()) {
            std::cout << Color::BRIGHT_RED
                      << "\n  Not enough coins! You need " << item->getPrice()
                      << " but only have " << inv.getCurrency() << ".\n"
                      << Color::RESET;
            std::cout << "\nPress Enter to continue...";
            std::cin.ignore(std::numeric_limits<std::streamsize>::max(), '\n');
            continue;
        }

        inv.spendCurrency(item->getPrice());
        inv.addItem(item, 1);
        std::cout << Color::BRIGHT_GREEN
                  << "\n  Bought " << item->getName() << " for "
                  << item->getPrice() << " coins!\n"
                  << "  You now have " << inv.getCurrency() << " coins.\n"
                  << Color::RESET;

        std::cout << "\nPress Enter to continue...";
        std::cin.ignore(std::numeric_limits<std::streamsize>::max(), '\n');
    }
}
