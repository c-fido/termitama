#pragma once
#include "Pet.h"
#include <string>
#include <memory>

class IFSMState {
public:
    virtual ~IFSMState() = default;
    virtual FSMState    stateId()      const = 0;
    virtual std::string name()         const = 0;
    virtual bool        canInteract()  const = 0;
    virtual FSMState    evaluate(const Pet& pet) const = 0;
    virtual std::string description()  const = 0;
};


class IdleState : public IFSMState {
public:
    FSMState    stateId()     const override { return FSMState::Idle; }
    std::string name()        const override { return "Idle"; }
    bool        canInteract() const override { return true; }
    FSMState    evaluate(const Pet& pet) const override;
    std::string description() const override { return "Awaiting your care"; }
};

class SleepingState : public IFSMState {
public:
    FSMState    stateId()     const override { return FSMState::Sleeping; }
    std::string name()        const override { return "Sleeping"; }
    bool        canInteract() const override { return false; }
    FSMState    evaluate(const Pet& pet) const override;
    std::string description() const override { return "z Z z Z  (Pet collapsed from exhaustion)"; }
};

class PlayingState : public IFSMState {
public:
    FSMState    stateId()     const override { return FSMState::Playing; }
    std::string name()        const override { return "Playing"; }
    bool        canInteract() const override { return true; }
    FSMState    evaluate(const Pet& pet) const override;
    std::string description() const override { return "Having fun!"; }
};

class SickState : public IFSMState {
public:
    FSMState    stateId()     const override { return FSMState::Sick; }
    std::string name()        const override { return "Sick"; }
    bool        canInteract() const override { return true; }
    FSMState    evaluate(const Pet& pet) const override;
    std::string description() const override { return "Feeling unwell — use a Cure-All!"; }
};

class DeadState : public IFSMState {
public:
    FSMState    stateId()     const override { return FSMState::Dead; }
    std::string name()        const override { return "Dead"; }
    bool        canInteract() const override { return false; }
    FSMState    evaluate(const Pet&) const override { return FSMState::Dead; }
    std::string description() const override { return "..."; }
};

class PetFSM {
public:
    explicit PetFSM(Pet& pet);

    std::string tick();

    std::string transitionTo(FSMState newState);

    FSMState    getState()    const;
    bool        canInteract() const { return current_->canInteract(); }
    std::string getStateName() const { return current_->name(); }
    std::string getStateDesc() const { return current_->description(); }

    void syncFromPet();

private:
    Pet&                      pet_;
    std::unique_ptr<IFSMState> current_;

    static std::unique_ptr<IFSMState> makeState(FSMState s);

    std::string doTransition(FSMState newState);
};
