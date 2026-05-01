#pragma once
#include "Item.h"
#include <unordered_map>
#include <memory>
#include <vector>
#include <string>
#include <sstream>

using ItemCatalog = std::unordered_map<std::string, std::shared_ptr<Item>>;

class Inventory {
public:
    Inventory() = default;

    int  getCurrency()        const { return currency_; }
    void addCurrency(int amt)       { currency_ += amt; }
    bool spendCurrency(int amt);

    void addItem(std::shared_ptr<Item> item, int qty = 1);
    bool removeItem(const std::string& id, int qty = 1);
    bool hasItem(const std::string& id)    const;
    int  getQuantity(const std::string& id) const;

    std::vector<std::pair<std::shared_ptr<Item>, int>> getAllItems() const;

    void appendToJson(std::ostringstream& ss)                             const;
    void loadFromJson(const std::string& json, const ItemCatalog& catalog);

private:
    int currency_ = 100;

    std::unordered_map<std::string, std::pair<std::shared_ptr<Item>, int>> items_;
};
