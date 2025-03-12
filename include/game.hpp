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
    std::span<int> rolls;
    std::span<Move> legal_moves;
    std::span<std::optional<Move>> registered_moves;
};

class Scorepad {
public:
    Scorepad();
    void mark_move(const Move& move);
    inline bool mark_penalty();

    std::optional<size_t> get_rightmost_mark_index(Color color) const {
        return m_rightmost_mark_indices[static_cast<size_t>(color)];
    }

    int get_value_from_index(Color color, size_t index) const {
        return m_rows[static_cast<size_t>(color)][index].first;
    }

    constexpr size_t get_index_from_value(Color color, int value) const {
        if (color == Color::red || color == Color::yellow) {
            return value - 2;
        }
        else {
            return 12 - value;
        }
    }

    int get_num_marks(Color color) const {
        return m_num_marks[static_cast<size_t>(color)];
    }

    int get_num_penalties() const {
        return m_penalties;
    }

private:
    std::array<std::array<std::pair<int, bool>, GameConstants::NUM_CELLS_PER_ROW>, GameConstants::NUM_ROWS> m_rows;
    std::array<std::optional<size_t>, GameConstants::NUM_ROWS> m_rightmost_mark_indices;
    std::array<int, GameConstants::NUM_ROWS> m_num_marks;
    int m_penalties;
};

struct State {
    std::vector<Scorepad> scorepads;
    std::bitset<GameConstants::NUM_ROWS> locks;
    size_t curr_player;
    int num_locks;
    bool is_terminal;

    State(size_t num_players, size_t starting_player);
};

class Agent {
public:
    std::optional<size_t> make_move(std::span<const Move> moves) const;
};

struct GameData {
    std::vector<size_t> winners;
    std::vector<int> final_score;
    std::unique_ptr<State> final_state;
};

class Game {
public:
    Game(size_t num_players);
    std::unique_ptr<GameData> run();
    std::vector<int> compute_score() const;
private:
    size_t m_num_players;
    std::unique_ptr<State> m_state;
    std::vector<Agent> m_players;

    template <ActionType A, typename F>
    bool resolve_action(const MoveContext& ctxt, F lock_added);
};

void roll_dice(std::span<int> rolls);

template <ActionType A>
size_t generate_legal_moves(const MoveContext& ctxt, const Scorepad& scorepad);