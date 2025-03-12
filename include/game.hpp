#pragma once

#include <array>
#include <bitset>
#include <functional>
#include <iostream>
#include <limits>
#include <memory>
#include <optional>
#include <span>
#include <vector>

namespace GameConstants {
    static constexpr size_t MIN_PLAYERS = 2;
    static constexpr size_t MAX_PLAYERS = 5;
    static constexpr size_t NUM_ROWS = 4;
    static constexpr size_t NUM_CELLS_PER_ROW = 11;
    static constexpr int NUM_DICE = 6;
    static constexpr size_t MAX_LEGAL_MOVES = 8;
    static constexpr int MIN_MARKS_FOR_LOCK = 5;
    static constexpr int MAX_PENALTIES = 4;
    static constexpr int NO_MARK = std::numeric_limits<int>::min();
}

enum class ActionType {
    First,
    Second
};

enum class Color { 
    red = 0,
    yellow = 1,
    green = 2,
    blue = 3
};

struct Move {
    Color color;
    size_t index;
};

struct MoveContext {
    std::span<Color> dice;
    std::span<unsigned int> rolls;  // TODO: consider changing to int and verifying that it must be in the range of 1 to 6 through other means
    std::span<Move> legalMoves;
    std::span<std::optional<Move>> registeredMoves;
};

class Scorepad {
public:
    Scorepad();
    void markMove(const Move& move);
    bool markPenalty();
    unsigned int getRightmostMarkValue(Color color) const {     // TODO: consider int instead of unsigned int
        return m_rightmostMarks[static_cast<size_t>(color)];
    }
    unsigned int getValueFromIndex(Color color, size_t index) const {   // TODO: " "
        return m_rows[static_cast<size_t>(color)][index].first;
    }
    constexpr size_t getIndexFromValue(Color color, unsigned int value) const {
        if (color == Color::red || color == Color::yellow) {
            return value - 2;
        }
        else {
            return 12 - value;
        }
    }
    unsigned int getNumMarks(Color color) const {   // TODO: " "
        return m_numMarks[static_cast<size_t>(color)];
    }
    int getNumPenalties() const {
        return m_penalties;
    }

private:
    std::array<std::array<std::pair<unsigned int, bool>, GameConstants::NUM_CELLS_PER_ROW>, GameConstants::NUM_ROWS> m_rows;    // TOOD: " "
    std::array<unsigned int, GameConstants::NUM_ROWS> m_rightmostMarks;     // TODO: " "
    std::array<unsigned int, GameConstants::NUM_ROWS> m_numMarks;   // TODO: " "
    int m_penalties;
};

struct State {
    std::vector<Scorepad> scorepads;
    std::bitset<GameConstants::NUM_ROWS> locks;
    size_t currPlayer;
    int numLocks;
    bool isTerminal;

    State(size_t numPlayers, size_t startingPlayer);
};

struct GameData {
    std::vector<size_t> winners;
    std::vector<int> finalScore;
    std::unique_ptr<State> finalState;
};

class Agent {
public:
    std::optional<size_t> makeMove(std::span<const Move> moves, const State& state) const;
};

class Game {
public:
    Game(size_t numPlayers);
    std::unique_ptr<GameData> run();
    void computeScore(std::vector<int>&);
private:
    size_t m_numPlayers;
    std::unique_ptr<State> m_state;
    std::vector<Agent> m_players;

    template <ActionType A, typename F>
    bool resolveAction(const MoveContext& ctxt, F lockAdded);
};