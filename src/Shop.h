#pragma once
#include "Inventory.h"
#include "Pet.h"
#include <string>

class Shop {
public:
    static const ItemCatalog& catalog();

    static std::shared_ptr<Item> getItem(const std::string& id);

    static std::string purchase(Inventory& inv, const std::string& itemId);

    // Legacy blocking-terminal shop loop (no longer called from TUI).
    static void run(Inventory& inv, const std::string& petName);

private:
    static void printCatalog(const Inventory& inv);
};
