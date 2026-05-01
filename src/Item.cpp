#include "Item.h"
#include "Pet.h"
#include <sstream>

std::string FoodItem::apply(Pet& pet) const {
    pet.applyFoodEffect(hungerRestore_, happinessBonus_);
    std::ostringstream ss;
    ss << pet.getName() << " enjoyed the " << name_ << "!"
       << " (+" << hungerRestore_ << " Hunger";
    if (happinessBonus_ > 0) ss << ", +" << happinessBonus_ << " Happiness";
    ss << ")";
    return ss.str();
}

std::string MedicineItem::apply(Pet& pet) const {
    pet.applyMedicine(healthRestore_, curesSickness_);
    std::ostringstream ss;
    ss << pet.getName() << " took " << name_ << "!";
    if (healthRestore_ > 0) ss << " (+" << healthRestore_ << " Health)";
    if (curesSickness_)     ss << " Sickness cured!";
    return ss.str();
}

std::string CosmeticItem::apply(Pet& pet) const {
    pet.receiveHappinessBoost(happinessBonus_);
    return pet.getName() + " loves the " + name_ + "! (+" +
           std::to_string(happinessBonus_) + " Happiness)";
}
