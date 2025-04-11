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

namespace GameConstants {
    static constexpr size_t MIN_PLAYERS = 2;
    static constexpr size_t MAX_PLAYERS = 5;
    static constexpr size_t NUM_ROWS = 4;
    static constexpr size_t NUM_CELLS_PER_ROW = 11;
    static constexpr size_t LOCK_INDEX = NUM_CELLS_PER_ROW - 1;
    static constexpr int NUM_DICE = 6;
    static constexpr size_t MAX_LEGAL_MOVES = 8;
    static constexpr int MIN_MARKS_FOR_LOCK = 5;
    static constexpr int MAX_PENALTIES = 4;
    static constexpr int PENALTY_VALUE = 5;
}

enum class ActionType {
    First,
    Second
};

// TODO: move to policy file
enum class PolicyType {
    Random,
    Greedy
};

enum class Color { 
    red = 0,
    yellow = 1,
    green = 2,
    blue = 3
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
    std::span<Move> legal_moves;
    std::span<std::optional<Move>> registered_moves;
};

class Scorepad {
public:
    Scorepad();
    void mark_move(const Move& move);

    inline bool mark_penalty() {
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
    // TODO: can this be changed to const int?
    std::array<std::array<std::pair<int, bool>, GameConstants::NUM_CELLS_PER_ROW>, GameConstants::NUM_ROWS> m_rows;
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

class Agent {
public:
    Agent(size_t position) : m_position(position) {};

    template <PolicyType P>
    std::optional<size_t> make_move(std::span<const Move> moves, const State& state) const;
protected:
    size_t m_position;
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

protected:
    size_t m_num_players;
    std::unique_ptr<State> m_state;
    std::vector<Agent> m_players;

    template <ActionType A, typename F>
    bool resolve_action(const MoveContext& ctxt, F lock_added);
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
size_t generate_legal_moves(const MoveContext& ctxt, const Scorepad& scorepad);

// This template function needs to be defined in the header file so as to avoid
// needing to instantiate a specific callable type
template <ActionType A, typename F>
bool Game::resolve_action(const MoveContext& ctxt, F lock_added) {
    bool active_player_made_move = false;

    if constexpr (A == ActionType::First) {
        //std::cout << "Resolving first action.\n";
        // Register first action moves
        for (size_t i = 0; i < m_num_players; ++i) {
            //std::cout << "Player " << i << " is making a move.\n";

            //std::cout << "Player " << i << "'s scorepad:\n" << m_state.get()->scorepads[i];
            
            const int num_moves = generate_legal_moves<ActionType::First>(ctxt, m_state.get()->scorepads[i]);

            std::optional<size_t> move_index_opt = std::nullopt;
            if (num_moves > 0) {
                // TODO: don't do this this way
                if (i == 0) {
                    move_index_opt = m_players[i].make_move<PolicyType::Random>(ctxt.legal_moves.subspan(0, num_moves), *m_state.get());
                }
                else {
                    move_index_opt = m_players[i].make_move<PolicyType::Greedy>(ctxt.legal_moves.subspan(0, num_moves), *m_state.get());
                }
            }

            if (move_index_opt.has_value()) {
                /*std::cout << "Player " << i << " has selected move " << "{ "
                          << color_to_string[ctxt.legal_moves[move_index_opt.value()].color]
                          << ' '
                          << index_to_value(ctxt.legal_moves[move_index_opt.value()].color, ctxt.legal_moves[move_index_opt.value()].index)
                          << " }\n";*/

                ctxt.registered_moves[i] = ctxt.legal_moves[move_index_opt.value()];
                if (i == m_state->curr_player) {
                    active_player_made_move = true;
                }
            }
            else {
                // SUBTLE BUG: without this, passes (no move) aren't registered, and
                // when ctxt.registered_moves is read later, the previous registered move
                // will be repeated!
                ctxt.registered_moves[i] = std::nullopt;
            }
        }

        // Make first action moves
        for (size_t i = 0; i < m_num_players; ++i) {
            const std::optional<Move> move_opt = ctxt.registered_moves[i];
            if (move_opt.has_value()) {
                //std::cout << "Calling mark_move for player " << i << '\n';
                m_state.get()->scorepads[i].mark_move(move_opt.value());
                //std::cout << "Player " << i << "'s scorepad after moving:\n" << m_state.get()->scorepads[i];
                if (move_opt.value().index == GameConstants::LOCK_INDEX) {
                    //std::cout << "The " << color_to_string[move_opt.value().color] << " row has been locked!";
                    m_state->locks[static_cast<size_t>(move_opt.value().color)] = true;
                }
            }
        }
        
        //std::cout << "First action resolved.\n";
    }
    else if constexpr (A == ActionType::Second) {
        /*std::cout << "Resolving second action.\n";

        std::cout << "Player " << m_state->curr_player << "'s scorepad:\n" << m_state.get()->scorepads[m_state->curr_player];*/
        
        const int num_moves = generate_legal_moves<ActionType::Second>(ctxt, m_state.get()->scorepads[m_state->curr_player]);

        std::optional<size_t> move_index_opt = std::nullopt;
        if (num_moves > 0) {
            // TODO: don't do this this way
            if (m_state.get()->curr_player == 0) {
                move_index_opt = m_players[m_state->curr_player].make_move<PolicyType::Random>(ctxt.legal_moves.subspan(0, num_moves), *m_state.get());
            }
            else {
                move_index_opt = m_players[m_state->curr_player].make_move<PolicyType::Greedy>(ctxt.legal_moves.subspan(0, num_moves), *m_state.get());
            }
        }
        if (move_index_opt.has_value()) {
            /*std::cout << "Active player" << " has selected move " << "{ "
            << color_to_string[ctxt.legal_moves[move_index_opt.value()].color]
            << ' '
            << index_to_value(ctxt.legal_moves[move_index_opt.value()].color, ctxt.legal_moves[move_index_opt.value()].index)
            << " }\n";*/

            //std::cout << "Calling mark_move for player " << m_state->curr_player << '\n';
            m_state.get()->scorepads[m_state->curr_player].mark_move(ctxt.legal_moves[move_index_opt.value()]);

            //std::cout << "Player " << m_state->curr_player << "'s scorepad after moving:\n" << m_state.get()->scorepads[m_state->curr_player];
            
            if (ctxt.legal_moves[move_index_opt.value()].index == GameConstants::LOCK_INDEX) {
                //std::cout << "The " << color_to_string[ctxt.legal_moves[move_index_opt.value()].color] << " row has been locked!"; 
                m_state->locks[static_cast<size_t>(ctxt.legal_moves[move_index_opt.value()].color)] = true;
            }
            active_player_made_move = true;
        }
        else {
            //std::cout << "Active player has opted to pass.\n";
        }
        //std::cout << "Second action resolved.\n";
    }

    // Check locks and remove corresponding dice
    if (m_state->locks.any()) {
        lock_added();
    }

    return active_player_made_move;
}

std::ostream& operator<< (std::ostream& stream, const MoveContext& ctxt);