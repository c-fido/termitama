#include "PetFSM.h"
#include <stdexcept>


FSMState IdleState::evaluate(const Pet& pet) const {
    if (!pet.isAlive())              return FSMState::Dead;
    if (pet.getStats().energy < 20.0f) return FSMState::Sleeping;
    if (pet.isSick())                return FSMState::Sick;
    return FSMState::Idle;
}

FSMState SleepingState::evaluate(const Pet& pet) const {
    if (!pet.isAlive())                return FSMState::Dead;
    if (pet.getStats().energy >= 65.0f) return FSMState::Idle;
    return FSMState::Sleeping;
}

FSMState PlayingState::evaluate(const Pet& pet) const {
    if (!pet.isAlive()) return FSMState::Dead;
    return FSMState::Idle;
}

FSMState SickState::evaluate(const Pet& pet) const {
    if (!pet.isAlive())                return FSMState::Dead;
    if (!pet.isSick())                 return FSMState::Idle;
    if (pet.getStats().energy < 20.0f) return FSMState::Sleeping;
    return FSMState::Sick;
}


PetFSM::PetFSM(Pet& pet)
    : pet_(pet), current_(makeState(pet.getFSMState()))
{}

std::unique_ptr<IFSMState> PetFSM::makeState(FSMState s) {
    switch (s) {
        case FSMState::Idle:     return std::make_unique<IdleState>();
        case FSMState::Sleeping: return std::make_unique<SleepingState>();
        case FSMState::Playing:  return std::make_unique<PlayingState>();
        case FSMState::Sick:     return std::make_unique<SickState>();
        case FSMState::Dead:     return std::make_unique<DeadState>();
    }
    return std::make_unique<IdleState>();
}

std::string PetFSM::doTransition(FSMState newState) {
    if (newState == current_->stateId()) return "";

    FSMState oldState = current_->stateId();
    current_ = makeState(newState);
    pet_.setFSMState(newState);

    const std::string& name = pet_.getName();
    switch (newState) {
        case FSMState::Sleeping:
            return Color::BLUE + name +
                   " collapsed from exhaustion and fell asleep! z Z z Z\n"
                   "(Wait for energy to recover before interacting.)" +
                   Color::RESET;
        case FSMState::Idle:
            if (oldState == FSMState::Sleeping)
                return Color::BRIGHT_GREEN + name + " woke up feeling refreshed!" + Color::RESET;
            if (oldState == FSMState::Sick)
                return Color::BRIGHT_GREEN + name + " has recovered! Back to normal." + Color::RESET;
            return "";
        case FSMState::Sick:
            return Color::BRIGHT_RED + name + " is now sick! Use a Cure-All from the Shop!" + Color::RESET;
        case FSMState::Playing:
            return "";
        case FSMState::Dead:
            return Color::BRIGHT_RED + name + " has passed away..." + Color::RESET;
    }
    return "";
}

std::string PetFSM::tick() {
    FSMState next = current_->evaluate(pet_);
    return doTransition(next);
}

std::string PetFSM::transitionTo(FSMState newState) {
    return doTransition(newState);
}

FSMState PetFSM::getState() const {
    return current_->stateId();
}

void PetFSM::syncFromPet() {
    current_ = makeState(pet_.getFSMState());
}
