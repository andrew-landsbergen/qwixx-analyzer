#include "agent.hpp"
#include "game.hpp"
#include "rng.hpp"

// TODO: double check how const works for containers
std::optional<size_t> RandomAgent::make_move(std::span<const Move> moves, const State& state) const {        
    (void) state;
    std::uniform_int_distribution<size_t> dist(0, moves.size());
    const size_t move_index = dist(rng());
    return move_index == 0 ? std::nullopt : std::optional<size_t>(move_index - 1);
};

std::optional<size_t> GreedyAgent::make_move(std::span<const Move> moves, const State& state) const {
    std::optional<size_t> choice = std::nullopt;
    int fewest_skips_seen = std::numeric_limits<int>::max();
    for (size_t i = 0; i < moves.size(); ++i) {
        const Color move_color = moves[i].color;
        const size_t move_index = moves[i].index;
        const std::optional<size_t> rightmost_index = state.scorepads[m_position].get_rightmost_mark_index(move_color);
        int num_skips = static_cast<int>(move_index);    // default if there is no rightmost index
        if (rightmost_index.has_value()) {
            // Invariant (assuming rest of code is correct): move_index > rightmost_index,
            // so skipped >= 0
            num_skips = static_cast<int>(move_index - rightmost_index.value()) - 1;
        }
        if (num_skips <= m_max_skips && num_skips < fewest_skips_seen) {
            choice = i;
        }
    }
    return choice;
}