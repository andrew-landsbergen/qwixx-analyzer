#include <chrono>
#include <random>

#include "rng.hpp"

/**
 * @brief Function that creates one instance of a global random number generator for use in all files, and then returns it.
 * @details Uses a Mersenne Twister from the standard library, seeded using the current time.
 * @return The random number generator instance.
 */
std::mt19937_64& rng() {
    static std::mt19937_64 rng;
    rng.seed(std::chrono::system_clock::now().time_since_epoch().count());
    return rng;
}