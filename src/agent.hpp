#pragma once

#include <memory>
#include <optional>
#include <span>

struct Move;
class State;

class Agent {
public:
    Agent() : m_position(0) {};

    virtual ~Agent() = default;
    
    virtual std::optional<size_t> make_move(std::span<const Move> current_action_legal_moves, std::span<const Move> action_two_possible_moves, const State& state) const = 0;

    void set_position(size_t position) {
        m_position = position;
    }
protected:
    size_t m_position;
};

class Human : public Agent {
public:
    Human() : Agent() {};

    std::optional<size_t> make_move(std::span<const Move> current_action_legal_moves, std::span<const Move> action_two_possible_moves, const State& state) const override;
};
    
class Random : public Agent {
public:
    Random() : Agent() {};

    std::optional<size_t> make_move(std::span<const Move> current_action_legal_moves, std::span<const Move> action_two_possible_moves, const State& state) const override;
};

class Greedy : public Agent {
public:
    Greedy(int max_skips) : Agent(), m_max_skips(max_skips) {};

    std::optional<size_t> make_move(std::span<const Move> current_action_legal_moves, std::span<const Move> action_two_possible_moves, const State& state) const override;
protected:
    int m_max_skips;
};

class GreedySkipLowProbability : public Agent {
public:
    GreedySkipLowProbability(int max_skips) : Agent(), m_max_skips(max_skips) {};

    std::optional<size_t> make_move(std::span<const Move> current_action_legal_moves, std::span<const Move> action_two_possible_moves, const State& state) const override;
protected:
    int m_max_skips;
};

class RushLocks : public Agent {
public:
    RushLocks(int two_skip_percent) : Agent(), m_two_skip_percent(two_skip_percent) {};

    std::optional<size_t> make_move(std::span<const Move> current_action_legal_moves, std::span<const Move> action_two_possible_moves, const State& state) const override;
protected:
    int m_two_skip_percent;
};