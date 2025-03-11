#pragma once

#include <array>
#include <bitset>
#include <functional>
#include <iostream>
#include <memory>
#include <optional>
#include <span>
#include <vector>

static const size_t MIN_PLAYERS = 2;
static const size_t MAX_PLAYERS = 5;
static const size_t NUM_ROWS = 4;
static const size_t NUM_CELLS_PER_ROW = 11;
static const int MAX_PENALTIES = 4;
static const unsigned int NUM_DICE = 6;
static const size_t MAX_LEGAL_MOVES = 8;
static const unsigned int MIN_MARKS_FOR_LOCK = 5;

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
    Color row;
    size_t index;
};

struct MoveContext {
    std::span<Color> dice;
    std::span<unsigned int> rolls;
    std::span<Move> legalMoves;
    std::span<std::optional<Move>> registeredMoves;
};

class Scorepad {
public:
    Scorepad();
    void markMove(const Move& move);
    bool markPenalty();
    inline unsigned int getRightmostMarkValue(Color color) const {
        return m_rightmostMarks[static_cast<size_t>(color)];
    }
    inline unsigned int getValueFromIndex(Color color, size_t index) const {
        return m_rows[static_cast<size_t>(color)][index].first;
    }
    inline size_t getIndexFromValue(Color color, unsigned int value) const {
        if (color == Color::red || color == Color::yellow) {
            return value - 2;
        }
        else {
            return 12 - value;
        }
    }
    inline unsigned int getNumMarks(Color color) const {
        return m_numMarks[static_cast<size_t>(color)];
    }
    inline int getNumPenalties() const {
        return m_penalties;
    }

private:
    std::array<std::array<std::pair<unsigned int, bool>, NUM_CELLS_PER_ROW>, NUM_ROWS> m_rows;
    std::array<unsigned int, NUM_ROWS> m_rightmostMarks;
    std::array<unsigned int, NUM_ROWS> m_numMarks; 
    int m_penalties;
};

struct State {
    std::vector<std::unique_ptr<Scorepad>> scorepads;
    std::bitset<NUM_ROWS> locks;
    size_t currPlayer;
    int numLocks;
    bool terminal;

    State(size_t numPlayers, size_t startingPlayer);
};

struct GameData {
    size_t winner;
    std::vector<int> finalScore;
    std::unique_ptr<State> finalState;
};

class Agent {
public:
    std::optional<size_t> makeMove(std::span<const Move> moves, const State& state);
};

class Game {
public:
    Game(size_t numPlayers);
    std::unique_ptr<GameData> run();
    void computeScore(std::vector<int>&);
private:
    size_t m_numPlayers;
    std::unique_ptr<State> m_state;
    std::vector<std::unique_ptr<Agent>> m_players;

    template <ActionType A>
    bool resolveAction(const MoveContext& ctxt, std::function<void()> lockAdded);
};