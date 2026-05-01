#include "Genetics.h"
#include "Pet.h"
#include <algorithm>
#include <chrono>

float Genetics::blendWithMutation(float a, float b, float mutRange,
                                  std::mt19937& rng) {
    float base = (a + b) * 0.5f;
    std::uniform_real_distribution<float> dist(-mutRange, mutRange);
    float mutated = base * (1.0f + dist(rng));
    return std::max(50.0f, std::min(mutated, 150.0f));
}

int Genetics::evolveThreshold(int stageIndex, int variant) {
    static const int T[3][3] = {
        {15, 40, 80},
        {10, 25, 50},
        {20, 55, 110},
    };
    int v = std::max(0, std::min(variant, 2));
    int s = std::max(0, std::min(stageIndex, 2));
    return T[v][s];
}

std::unique_ptr<Pet> Genetics::breed(const Pet& parentA,
                                     const Pet& parentB,
                                     const std::string& offspringName) {
    if (parentA.getStage() != EvolutionStage::Adult ||
        parentB.getStage() != EvolutionStage::Adult)
        return nullptr;

    std::mt19937 rng(static_cast<unsigned>(
        std::chrono::high_resolution_clock::now().time_since_epoch().count()));

    const Genome& ga = parentA.getGenome();
    const Genome& gb = parentB.getGenome();

    Genome offspring;
    offspring.maxHunger       = blendWithMutation(ga.maxHunger,    gb.maxHunger,    0.10f, rng);
    offspring.maxHappiness    = blendWithMutation(ga.maxHappiness, gb.maxHappiness, 0.10f, rng);
    offspring.maxEnergy       = blendWithMutation(ga.maxEnergy,    gb.maxEnergy,    0.10f, rng);
    offspring.maxHealth       = blendWithMutation(ga.maxHealth,    gb.maxHealth,    0.10f, rng);
    float dr = ((ga.decayResistance + gb.decayResistance) * 0.5f);
    std::uniform_real_distribution<float> smallDist(-0.05f, 0.05f);
    offspring.decayResistance = std::max(0.5f, std::min(dr * (1.0f + smallDist(rng)), 1.5f));
    std::uniform_int_distribution<int> coinFlip(0, 1);
    offspring.evolutionVariant = coinFlip(rng)
                               ? ga.evolutionVariant
                               : gb.evolutionVariant;

    auto baby = std::make_unique<Pet>(offspringName);
    baby->setGenome(offspring);
    return baby;
}
