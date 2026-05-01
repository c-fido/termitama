#include "Inventory.h"
#include <algorithm>

static std::string invJsonGet(const std::string& json, const std::string& key) {
    std::string needle = "\"" + key + "\"";
    auto pos = json.find(needle);
    if (pos == std::string::npos) return "";
    pos = json.find(':', pos + needle.size());
    if (pos == std::string::npos) return "";
    ++pos;
    while (pos < json.size() &&
           (json[pos] == ' ' || json[pos] == '\n' || json[pos] == '\r')) ++pos;
    if (pos >= json.size()) return "";
    if (json[pos] == '"') {
        auto end = json.find('"', pos + 1);
        return (end == std::string::npos) ? "" : json.substr(pos + 1, end - pos - 1);
    }
    auto end = json.find_first_of(",}\n", pos);
    std::string tok = (end == std::string::npos)
                    ? json.substr(pos) : json.substr(pos, end - pos);
    while (!tok.empty() && (tok.back() == ' ' || tok.back() == '\r' || tok.back() == '\n'))
        tok.pop_back();
    return tok;
}

bool Inventory::spendCurrency(int amt) {
    if (currency_ < amt) return false;
    currency_ -= amt;
    return true;
}

void Inventory::addItem(std::shared_ptr<Item> item, int qty) {
    auto& entry = items_[item->getId()];
    if (!entry.first) entry.first = item;
    entry.second += qty;
}

bool Inventory::removeItem(const std::string& id, int qty) {
    auto it = items_.find(id);
    if (it == items_.end() || it->second.second < qty) return false;
    it->second.second -= qty;
    if (it->second.second == 0) items_.erase(it);
    return true;
}

bool Inventory::hasItem(const std::string& id) const {
    auto it = items_.find(id);
    return it != items_.end() && it->second.second > 0;
}

int Inventory::getQuantity(const std::string& id) const {
    auto it = items_.find(id);
    return (it != items_.end()) ? it->second.second : 0;
}

std::vector<std::pair<std::shared_ptr<Item>, int>> Inventory::getAllItems() const {
    std::vector<std::pair<std::shared_ptr<Item>, int>> result;
    result.reserve(items_.size());
    for (const auto& [id, entry] : items_)
        result.emplace_back(entry.first, entry.second);
    std::sort(result.begin(), result.end(),
              [](const auto& a, const auto& b) {
                  return a.first->getName() < b.first->getName();
              });
    return result;
}

void Inventory::appendToJson(std::ostringstream& ss) const {
    ss << "  \"currency\": " << currency_ << ",\n";
    for (const auto& [id, entry] : items_) {
        if (entry.second > 0)
            ss << "  \"inv_" << id << "\": " << entry.second << ",\n";
    }
}

void Inventory::loadFromJson(const std::string& json, const ItemCatalog& catalog) {
    std::string cur = invJsonGet(json, "currency");
    if (!cur.empty()) currency_ = std::stoi(cur);

    for (const auto& [id, item] : catalog) {
        std::string val = invJsonGet(json, "inv_" + id);
        if (!val.empty()) {
            int qty = std::stoi(val);
            if (qty > 0) items_[id] = {item, qty};
        }
    }
}
