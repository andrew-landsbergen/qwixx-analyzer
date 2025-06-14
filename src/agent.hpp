#pragma once

#include <memory>
#include <optional>
#include <span>

#include "globals.hpp"

struct Move;
class State;

class Agent {
public:
    Agent() : m_position(0) {};

    virtual ~Agent() = default;
    
    virtual std::optional<size_t> make_move(bool first_action, std::span<const Move> current_action_legal_moves, std::span<const Move> action_two_possible_moves, const State& state) = 0;

    void set_position(size_t position) {
        m_position = position;
    }
protected:
    size_t m_position;
};

class Human : public Agent {
public:
    Human() : Agent() {};

    std::optional<size_t> make_move(bool first_action, std::span<const Move> current_action_legal_moves, std::span<const Move> action_two_possible_moves, const State& state) override;
};
    
class Random : public Agent {
public:
    Random() : Agent() {};

    std::optional<size_t> make_move(bool first_action, std::span<const Move> current_action_legal_moves, std::span<const Move> action_two_possible_moves, const State& state) override;
};

class Greedy : public Agent {
public:
    Greedy(int max_skips) : Agent(), m_max_skips(max_skips) {};

    std::optional<size_t> make_move(bool first_action, std::span<const Move> current_action_legal_moves, std::span<const Move> action_two_possible_moves, const State& state) override;
protected:
    int m_max_skips;
};

class GreedyImproved : public Agent {
public:
    GreedyImproved(int max_skips) : Agent(), m_standard_max_skips(max_skips) {};

    std::optional<size_t> make_move(bool first_action, std::span<const Move> current_action_legal_moves, std::span<const Move> action_two_possible_moves, const State& state) override;
protected:
    bool m_made_first_action_move = false;    
    int m_standard_max_skips;
    int m_standard_max_penalty_avoidance_skips = 1;
};

class RushLocks : public Agent {
public:
    RushLocks(int two_skip_percent) : Agent(), m_two_skip_percent(two_skip_percent) {};

    std::optional<size_t> make_move(bool first_action, std::span<const Move> current_action_legal_moves, std::span<const Move> action_two_possible_moves, const State& state) override;
protected:
    int m_two_skip_percent;
};

class Computational : public Agent {
public:
    Computational();

    struct MoveData {
        int base_penalty;
        int roll_frequency;
    };

    std::optional<size_t> make_move(bool first_action, std::span<const Move> current_action_legal_moves, std::span<const Move> action_two_possible_moves, const State& state) override;
protected:
    bool m_made_first_action_move = false;
    double m_alpha = 0.95;
    double m_mu = 0.5;
    double m_delta = 0.84;
    double m_sigma = 0.95;
    double m_epsilon = 0.70;
    std::array<MoveData, GameConstants::NUM_CELLS_PER_ROW> m_basic_values;
};