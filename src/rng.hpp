#pragma once

#include <random>

/**
 * @brief Function that returns a global random number generator for use in all files.
 */
extern std::mt19937_64& rng();