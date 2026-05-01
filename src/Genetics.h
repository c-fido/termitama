#pragma once
#include "Pet.h"
#include <memory>
#include <string>
#include <random>

class Genetics {
public:
    static std::unique_ptr<Pet> breed(const Pet& parentA,
                                      const Pet& parentB,
                                      const std::string& offspringName);

    static int evolveThreshold(int stageIndex, int variant);

private:
    static float blendWithMutation(float a, float b, float mutRange,
                                   std::mt19937& rng);
};
