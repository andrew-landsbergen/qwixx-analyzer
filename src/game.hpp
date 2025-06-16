#pragma once

#include <array>
#include <bitset>
#include <functional>
#include <iostream>
#include <limits>
#include <map>
#include <memory>
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
    size_t curr_player;
    int num_locks;
    bool is_terminal;

    State(size_t num_players, size_t starting_player) :
        scorepads(num_players, Scorepad()),
        locks(false),
        curr_player(starting_player),
        num_locks(0),
        is_terminal(false)
        {};
};

struct GameData {
    std::vector<size_t> winners;
    std::vector<int> final_score;
    std::unique_ptr<State> final_state;
};

class Game {
public:
    Game(std::vector<Agent*> players, bool human_active);
    std::unique_ptr<GameData> run();
    std::vector<int> compute_score() const;

protected:
    size_t m_num_players;
    std::unique_ptr<State> m_state;
    std::vector<Agent*> m_players;
    bool m_human_active;

    template <ActionType A, typename F>
    bool resolve_action(MoveContext& ctxt, F lock_added);
};

constexpr int index_to_value(Color color, size_t index) {
    if (color == Color::red || color == Color::yellow) {
        return static_cast<int>(index + 2);
    }
    else {
        return static_cast<int>(12 - index);
    }
};

constexpr size_t value_to_index(Color color, int value) {
    if (color == Color::red || color == Color::yellow) {
        return static_cast<size_t>(value) - 2;
    }
    else {
        return 12 - static_cast<size_t>(value);
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