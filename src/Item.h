#pragma once
#include <string>
#include <memory>

class Pet;

class Item {
public:
    Item(std::string id, std::string name, std::string desc, int price)
        : id_(std::move(id)), name_(std::move(name))
        , desc_(std::move(desc)), price_(price) {}

    virtual ~Item() = default;

    virtual std::string apply(Pet& pet) const = 0;
    virtual std::string getType()       const = 0;

    const std::string& getId()          const { return id_; }
    const std::string& getName()        const { return name_; }
    const std::string& getDescription() const { return desc_; }
    int                getPrice()       const { return price_; }

protected:
    std::string id_, name_, desc_;
    int         price_;
};

class FoodItem : public Item {
public:
    FoodItem(std::string id, std::string name, std::string desc, int price,
             int hungerRestore, int happinessBonus)
        : Item(std::move(id), std::move(name), std::move(desc), price)
        , hungerRestore_(hungerRestore), happinessBonus_(happinessBonus) {}

    std::string apply(Pet& pet) const override;
    std::string getType()       const override { return "Food"; }

    int getHungerRestore()  const { return hungerRestore_; }
    int getHappinessBonus() const { return happinessBonus_; }

private:
    int hungerRestore_, happinessBonus_;
};

class MedicineItem : public Item {
public:
    MedicineItem(std::string id, std::string name, std::string desc, int price,
                 int healthRestore, bool curesSickness)
        : Item(std::move(id), std::move(name), std::move(desc), price)
        , healthRestore_(healthRestore), curesSickness_(curesSickness) {}

    std::string apply(Pet& pet) const override;
    std::string getType()       const override { return "Medicine"; }

    int  getHealthRestore()  const { return healthRestore_; }
    bool getCuresSickness()  const { return curesSickness_; }

private:
    int  healthRestore_;
    bool curesSickness_;
};

class CosmeticItem : public Item {
public:
    CosmeticItem(std::string id, std::string name, std::string desc, int price,
                 int happinessBonus)
        : Item(std::move(id), std::move(name), std::move(desc), price)
        , happinessBonus_(happinessBonus) {}

    std::string apply(Pet& pet) const override;
    std::string getType()       const override { return "Cosmetic"; }

private:
    int happinessBonus_;
};
