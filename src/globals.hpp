#pragma once

namespace GameConstants {
    static constexpr size_t MIN_PLAYERS = 2;
    static constexpr size_t MAX_PLAYERS = 5;
    static constexpr size_t NUM_ROWS = 4;
    static constexpr size_t NUM_CELLS_PER_ROW = 11;
    static constexpr size_t LOCK_INDEX = NUM_CELLS_PER_ROW - 1;
    static constexpr int NUM_DICE = 6;
    static constexpr size_t MAX_LEGAL_MOVES = 8;
    static constexpr int MIN_MARKS_FOR_LOCK = 5;
    static constexpr int MAX_PENALTIES = 4;
    static constexpr int PENALTY_VALUE = 5;
}

enum class Color { 
    red = 0,
    yellow = 1,
    green = 2,
    blue = 3
};