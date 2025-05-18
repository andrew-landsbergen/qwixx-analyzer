#include "agent.hpp"
#include "game.hpp"
#include "rng.hpp"

// TODO: double check how const works for containers
std::optional<size_t> Random::make_move(std::span<const Move> moves, const State& state) const {        
    (void) state;
    std::uniform_int_distribution<size_t> dist(0, moves.size());
    const size_t move_index = dist(rng());
    return move_index == 0 ? std::nullopt : std::optional<size_t>(move_index - 1);
};

std::optional<size_t> Greedy::make_move(std::span<const Move> moves, const State& state) const {
    std::optional<size_t> choice = std::nullopt;
    int fewest_skips_seen = std::numeric_limits<int>::max();
    for (size_t i = 0; i < moves.size(); ++i) {
        const Color move_color = moves[i].color;
        const size_t move_index = moves[i].index;
        const std::optional<size_t> rightmost_index = state.scorepads[m_position].get_rightmost_mark_index(move_color);

        int num_skips = static_cast<int>(move_index);    // default if there is no rightmost index
        if (rightmost_index.has_value()) {
            // Invariant: move_index > rightmost_index, so num_skips >= 0
            num_skips = static_cast<int>(move_index - rightmost_index.value()) - 1;
        }
        if (num_skips <= m_max_skips && num_skips < fewest_skips_seen) {
            choice = i;
            fewest_skips_seen = num_skips;
        }
    }
    return choice;
}

std::optional<size_t> GreedySkipLowProbability::make_move(std::span<const Move> moves, const State& state) const {
    std::optional<size_t> choice = std::nullopt;
    int fewest_skips_seen = std::numeric_limits<int>::max();
    for (size_t i = 0; i < moves.size(); ++i) {
        const Color move_color = moves[i].color;
        const size_t move_index = moves[i].index;
        const std::optional<size_t> rightmost_index = state.scorepads[m_position].get_rightmost_mark_index(move_color);
        
        // Always mark locks if possible
        if (move_index == GameConstants::LOCK_INDEX) {
            return i;
        }

        int num_skips = static_cast<int>(move_index);    // default if there is no rightmost index
        if (rightmost_index.has_value()) {
            // Invariant: move_index > rightmost_index, so num_skips >= 0
            num_skips = static_cast<int>(move_index - rightmost_index.value()) - 1;
        }

        // Add 1 to the maximum number of skips if: no marks have been placed yet, the rightmost mark is on
        // the first or second cell, or we are marking a lock
        int effective_max_skips = m_max_skips;
        if (!rightmost_index.has_value() 
        || (rightmost_index.has_value() && rightmost_index.value() <= 1)) {
            effective_max_skips += 1;
        }

        // Still try to minimize number of skips
        if (num_skips <= effective_max_skips && num_skips < fewest_skips_seen) {
            choice = i;
            fewest_skips_seen = num_skips;
        }
    }
    return choice;
}

std::optional<size_t> RushLocks::make_move(std::span<const Move> moves, const State& state) const {    
    std::optional<size_t> choice = std::nullopt;
    
    /*for (size_t i = 0; i < moves.size(); ++i) {
        // Always mark lock if possible
        if (moves[i].index == GameConstants::LOCK_INDEX) {
            return i;
        }
    }*/

    int most_marks_seen = std::numeric_limits<int>::min();
    for (size_t i = 0; i < moves.size(); ++i) {
        const Color move_color = moves[i].color;
        const size_t move_index = moves[i].index;
        const std::optional<size_t> rightmost_index = state.scorepads[m_position].get_rightmost_mark_index(move_color);
        const int num_marks = state.scorepads[m_position].get_num_marks(move_color);

        int num_skips = static_cast<int>(move_index);    // default if there is no rightmost index
        if (rightmost_index.has_value()) {
            // Invariant: move_index > rightmost_index, so num_skips >= 0
            num_skips = static_cast<int>(move_index - rightmost_index.value()) - 1;
        }

        // If a 1-skip or 2-skip is available on a row with at least 3 marks placed, take it
        if (num_skips <= 2 && num_marks >= most_marks_seen) {
            choice = i;
            most_marks_seen = num_marks;
        }
    }

    if (choice.has_value()) {
        return choice;
    }

    int fewest_skips_seen = std::numeric_limits<int>::max();
    for (size_t i = 0; i < moves.size(); ++i) {
        const Color move_color = moves[i].color;
        const size_t move_index = moves[i].index;
        const std::optional<size_t> rightmost_index = state.scorepads[m_position].get_rightmost_mark_index(move_color);

        int num_skips = static_cast<int>(move_index);    // default if there is no rightmost index
        if (rightmost_index.has_value()) {
            // Invariant: move_index > rightmost_index, so num_skips >= 0
            num_skips = static_cast<int>(move_index - rightmost_index.value()) - 1;
        }

        // If a 1- or 2-skip is available, take it
        if (num_skips <= 2 && num_skips < fewest_skips_seen) {
            choice = i;
            fewest_skips_seen = num_skips;
        }
    }

    /*if (choice.has_value()) {
        std::uniform_int_distribution<size_t> dist(1, 100);
        const int roll = dist(rng());
        if (m_two_skip_percent >= roll) {
            return choice;
        }
        else {
            return std::nullopt;
        }
    }*/
   return choice;
}