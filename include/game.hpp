#pragma once

#include <array>
#include <iostream>
#include <optional>
#include <vector>

static const unsigned int NUM_ROWS = 4;
static const unsigned int NUM_CELLS_PER_ROW = 11;
static const unsigned int MAX_PENALTIES = 4;

enum class Color { 
    red = 0,
    yellow = 1,
    green = 2,
    blue = 3
};

struct Move {
    Color row;
    int index;
};

class Scorepad {
public:
    Scorepad() : m_rows(NUM_ROWS, std::vector<bool>(NUM_CELLS_PER_ROW, false)), m_rightmostMarks{}, m_penalties(0) {};
    void markNumberAtRow(const Move& move);
    bool markPenalty();
    int getRightmostMarkIndex(Color color) const;

private:
    std::vector<std::vector<bool>> m_rows;
    std::array<int, NUM_ROWS> m_rightmostMarks;
    int m_penalties;
};

struct State {
    const std::vector<std::unique_ptr<Scorepad>>& scorepads;
    int currPlayer;
    int numLocks;
    bool terminal;

    State(std::vector<std::unique_ptr<Scorepad>>& scorepads, int startingPlayer)
        : scorepads(scorepads),
          currPlayer(startingPlayer),
          numLocks(0),
          terminal(false)
    {}
};

class Agent {
public:
    int makeMove(Move **legalMoves, size_t numMoves, State *state);
};

class Game {
public:
    Game(int n);
    State *run();
private:
    std::unique_ptr<State> state;
    std::vector<std::unique_ptr<Agent>> players;
};