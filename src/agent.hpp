#pragma once

#include <memory>
#include <optional>
#include <span>

struct Move;
class State;

class Agent {
public:
    Agent(size_t position) : m_position(position) {};
    
    virtual std::optional<size_t> make_move(std::span<const Move> moves, const State& state) const = 0;
protected:
    size_t m_position;
};
    
class RandomAgent : public Agent {
public:
    RandomAgent(size_t position) : Agent(position) {};
    std::optional<size_t> make_move(std::span<const Move> moves, const State& state) const override;
};

class GreedyAgent : public Agent {
public:
    GreedyAgent(size_t position, int max_skips) : Agent(position), m_max_skips(max_skips) {};
    std::optional<size_t> make_move(std::span<const Move> moves, const State& state) const override;
protected:
    int m_max_skips;
};