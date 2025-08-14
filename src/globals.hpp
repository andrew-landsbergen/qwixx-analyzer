#pragma once

/**
 * @namespace GameConstants globals.hpp "src/globals.hpp"
 * @brief Constants related to the game of Qwixx.
 */
namespace GameConstants {
    static constexpr size_t MIN_PLAYERS = 2;
    static constexpr size_t MAX_PLAYERS = 5;
    static constexpr size_t NUM_ROWS = 4;           //< Scorepad rows.
    static constexpr size_t NUM_CELLS_PER_ROW = 11;
    static constexpr size_t LOCK_INDEX = NUM_CELLS_PER_ROW - 1;
    static constexpr int NUM_DICE = 6;
    static constexpr size_t MAX_LEGAL_MOVES = 8;    //< Maximum number of moves possible for either action.
    static constexpr int MIN_MARKS_FOR_LOCK = 5;    //< Minimum number of spaces that need to be marked in order to mark the lock on that row.
    static constexpr int MAX_PENALTIES = 4;
    static constexpr int PENALTY_VALUE = 5;
}

/**
 * @enum Color globals.hpp "src/globals.hpp"
 * @brief Colors of the rows and colored dice.
 * @attention The numerical values associated with each color are used throughout the code
 * for indexing, and so cannot be freely changed.
 */
enum class Color { 
    red = 0,
    yellow = 1,
    green = 2,
    blue = 3
};