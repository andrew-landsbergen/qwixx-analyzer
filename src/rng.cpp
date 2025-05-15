#include <chrono>
#include <random>

#include "rng.hpp"

std::mt19937_64& rng() {
    static std::mt19937_64 rng;
    rng.seed(std::chrono::system_clock::now().time_since_epoch().count());
    return rng;
}