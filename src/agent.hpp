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
    
    virtual std::optional<size_t> make_move(std::span<const Move> moves, const State& state) const = 0;

    void set_position(size_t position) {
        m_position = position;
    }
protected:
    size_t m_position;
};
    
class RandomAgent : public Agent {
public:
    RandomAgent() : Agent() {};

    std::optional<size_t> make_move(std::span<const Move> moves, const State& state) const override;
};

class GreedyAgent : public Agent {
public:
    GreedyAgent(int max_skips) : Agent(), m_max_skips(max_skips) {};

    std::optional<size_t> make_move(std::span<const Move> moves, const State& state) const override;
protected:
    int m_max_skips;
};