#pragma once

#include <array>
#include <bitset>
#include <functional>
#include <iostream>
#include <limits>
#include <map>
#include <memory>
#include <numeric>
#include <optional>
#include <span>
#include <vector>

#include "agent.hpp"
#include "globals.hpp"

enum class ActionType {
    First,
    Second
};

static std::map<Color, std::string> color_to_string = {
    { Color::red, "RED" },
    { Color::yellow, "YELLOW" },
    { Color::green, "GREEN" },
    { Color::blue, "BLUE" }
};

struct Move {
    Color color;
    size_t index;
};

struct MoveContext {
    std::span<Color> dice;
    std::span<int> rolls;
    std::span<Move> current_action_legal_moves;
    std::span<Move> action_two_possible_moves;
    std::span<std::optional<Move>> action_one_registered_moves;
};

class Scorepad {
public:
    Scorepad();
    void mark_move(const Move& move);

    bool mark_penalty() {
        return (++m_penalties >= GameConstants::MAX_PENALTIES);
    };

    std::optional<size_t> get_rightmost_mark_index(Color color) const {
        return m_rightmost_mark_indices[static_cast<size_t>(color)];
    }

    int get_num_marks(Color color) const {
        return m_num_marks[static_cast<size_t>(color)];
    }

    int get_num_penalties() const {
        return m_penalties;
    }

    friend std::ostream& operator<< (std::ostream& stream, const Scorepad& scorepad);

protected:
    std::array<std::array<bool, GameConstants::NUM_CELLS_PER_ROW>, GameConstants::NUM_ROWS> m_rows;
    std::array<std::optional<size_t>, GameConstants::NUM_ROWS> m_rightmost_mark_indices;
    std::array<int, GameConstants::NUM_ROWS> m_num_marks;
    int m_penalties;
};

// TODO: make this a class with some of its data protected?
struct State {
    std::vector<Scorepad> scorepads;
    std::bitset<GameConstants::NUM_ROWS> locks;
    std::array<bool, GameConstants::NUM_ROWS> locked_rows;      // may want to remove bitset if we use this
    size_t curr_player;
    int turn_count;
    int num_locks;
    bool is_terminal;

    State(size_t num_players, size_t starting_player) :
        scorepads(num_players, Scorepad()),
        locks(false),
        locked_rows{false, false, false, false},
        curr_player(starting_player),
        turn_count(0),
        num_locks(0),
        is_terminal(false)
        {};
};

struct GameData {
    std::vector<size_t> winners;
    std::vector<int> final_score;
    std::unique_ptr<State> final_state;
    std::vector<double> p0_evaluation_history;
    int num_turns;
};

class Game {
public:
    Game(std::vector<Agent*> players, bool human_active, bool use_evaluation);
    std::unique_ptr<GameData> run();
    std::vector<int> compute_score() const;
    double evaluate_2p();

protected:
    size_t m_num_players;
    std::unique_ptr<State> m_state;
    std::vector<Agent*> m_players;
    bool m_human_active;
    bool m_use_evaluation;

    //                                                                                      2/3  4  5  6  7  8  9  10 11/12 (or reverse)
    static constexpr std::array<int, GameConstants::NUM_CELLS_PER_ROW> m_frequency_counts = {3, 3, 4, 5, 6, 5, 4, 3, 3};
    static constexpr int m_max_frequency_count_left = GameConstants::NUM_ROWS * std::accumulate(m_frequency_counts.begin(), m_frequency_counts.end(), 0);
    
    double m_score_diff_weight;
    double m_freq_count_diff_weight;
    double m_lock_progress_diff_weight;
    double m_num_locks_weight;

    double m_score_diff_scale_factor;
    double m_freq_count_diff_scale_factor;
    double m_lock_progress_diff_scale_factor;
    double m_lock_progress_diff_bias;

    template <ActionType A, typename F>
    bool resolve_action(MoveContext& ctxt, F lock_added);
};

constexpr int index_to_value(Color color, size_t index) {
    if (color == Color::red || color == Color::yellow) {
        return static_cast<int>(index + 3);
    }
    else {
        return static_cast<int>(11 - index);
    }
};

constexpr size_t value_to_index(Color color, int value) {
    if (color == Color::red || color == Color::yellow) {
        if (value == 2) {
            return 0;
        }
        else if (value == 12) {
            return GameConstants::LOCK_INDEX;
        }
        else {
            return static_cast<size_t>(value) - 3;
        }
    }
    else {
        if (value == 12) {
            return 0;
        }
        else if (value == 2) {
            return GameConstants::LOCK_INDEX;
        }
        else {
            return 11 - static_cast<size_t>(value);
        }
        
    }
};

void roll_dice(std::span<int> rolls);

template <ActionType A>
size_t generate_legal_moves(std::span<Move>& legal_moves, const std::span<Color>& dice, const std::span<int>& rolls, const Scorepad& scorepad);

// This template function needs to be defined in the header file so as to avoid
// needing to instantiate a specific callable type
template <ActionType A, typename F>
bool Game::resolve_action(MoveContext& ctxt, F lock_added) {
    bool active_player_made_move = false;

    if constexpr (A == ActionType::First) {
        // Generate the currently possible action two moves.
        // This allows an agent to make its action one move on the
        // basis of its possible action one and action two moves.
        const int num_action_two_moves = generate_legal_moves<ActionType::Second>(ctxt.action_two_possible_moves, ctxt.dice, ctxt.rolls, m_state.get()->scorepads[m_state->curr_player]);
        
        // Register first action moves
        for (size_t i = 0; i < m_num_players; ++i) {            
            const int num_action_one_moves = generate_legal_moves<ActionType::First>(ctxt.current_action_legal_moves, ctxt.dice, ctxt.rolls, m_state.get()->scorepads[i]);

            std::optional<size_t> move_index_opt = std::nullopt;
            if (num_action_one_moves > 0) {
                move_index_opt = m_players[i]->make_move(true, ctxt.current_action_legal_moves.subspan(0, num_action_one_moves), 
                                                         ctxt.action_two_possible_moves.subspan(0, num_action_two_moves), *m_state.get());
            }

            if (move_index_opt.has_value()) {
                ctxt.action_one_registered_moves[i] = ctxt.current_action_legal_moves[move_index_opt.value()];
                if (i == m_state->curr_player) {
                    active_player_made_move = true;
                }
            }
            else {
                // SUBTLE BUG: without this, passes (no move) aren't registered, and
                // when ctxt.registered_moves is read later, the previous registered move
                // will be repeated!
                ctxt.action_one_registered_moves[i] = std::nullopt;
            }
        }

        // Make first action moves
        for (size_t i = 0; i < m_num_players; ++i) {
            const std::optional<Move> move_opt = ctxt.action_one_registered_moves[i];
            if (move_opt.has_value()) {
                m_state.get()->scorepads[i].mark_move(move_opt.value());
                if (move_opt.value().index == GameConstants::LOCK_INDEX) {
                    m_state->locks[static_cast<size_t>(move_opt.value().color)] = true;
                }
            }
        }
    }
    else if constexpr (A == ActionType::Second) {        
        // We do need to regenerate these moves, since some possible moves from before 
        // may no longer be possible after action one resolves
        const int num_moves = generate_legal_moves<ActionType::Second>(ctxt.current_action_legal_moves, ctxt.dice, ctxt.rolls, m_state.get()->scorepads[m_state->curr_player]);

        std::optional<size_t> move_index_opt = std::nullopt;
        if (num_moves > 0) {
            move_index_opt = m_players[m_state->curr_player]->make_move(false, ctxt.current_action_legal_moves.subspan(0, num_moves),
                                                                        ctxt.current_action_legal_moves.subspan(0, num_moves), *m_state.get());
        }
        if (move_index_opt.has_value()) {
            m_state.get()->scorepads[m_state->curr_player].mark_move(ctxt.current_action_legal_moves[move_index_opt.value()]);
            
            if (ctxt.current_action_legal_moves[move_index_opt.value()].index == GameConstants::LOCK_INDEX) {
                m_state->locks[static_cast<size_t>(ctxt.current_action_legal_moves[move_index_opt.value()].color)] = true;
            }
            active_player_made_move = true;
        }
    }

    // Check locks and remove corresponding dice
    if (m_state->locks.any()) {
        lock_added();
    }

    return active_player_made_move;
}

std::ostream& operator<< (std::ostream& stream, const MoveContext& ctxt);