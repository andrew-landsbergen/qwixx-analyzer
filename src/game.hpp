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

/**
 * @enum ActionType game.hpp "src/game.hpp"
 * @brief Used to denote whether the first or second action is being processed.
 */
enum class ActionType {
    First,
    Second
};

/**
 * @brief Mapping from color enum to the color's name as a string.
 * @attention It is possible to overwrite the string values associated with each key,
 * e.g. by color_to_string[Color::red] = "YELLOW".
 * @details Used for pretty printing.
 */
static std::map<Color, std::string> color_to_string = {
    { Color::red, "RED" },
    { Color::yellow, "YELLOW" },
    { Color::green, "GREEN" },
    { Color::blue, "BLUE" }
};

/**
 * @struct Move game.hpp "src/game.hpp"
 * @brief Implements a Qwixx move.
 * @details In Qwixx, moves have a color (red, yellow, green, or blue) and an index
 * (position along the row of the corresponding color in the scorepad).
 */
struct Move {
    Color color;
    size_t index;
};

/**
 * @struct MoveContext game.hpp "src/game.hpp"
 * @brief Holds contextual information related to the current move.
 * @details While an action is being resolved and agents are making their moves,
 * the available dice and their rolls remain fixed. generate_legal_moves() is the
 * function responsible for populating current_action_legal_moves and
 * action_two_possible_moves. When agents make their moves during the first action,
 * they are registered in action_one_registered_moves before being committed once
 * the action fully resolves. This is to prevent agents from using the actions of
 * other agents during the first action to inform their own move.
 */
struct MoveContext {
    std::span<Color> dice;      //< Span of the colors that still have a corresponding die that can be rolled.
    std::span<int> rolls;       //< Values of the dice rolls. The first two elements are for the white dice.
    std::span<Move> current_action_legal_moves;     //< The moves that are currently possible. Filled by generate_legal_moves().
    std::span<Move> action_two_possible_moves;      //< The moves that will be possible during action two. Filled by generate_legal_moves().
    std::span<std::optional<Move>> action_one_registered_moves;     //< The moves registered by each agent for the first action.
};

/**
 * @class Scorepad game.hpp "src/game.hpp"
 * @brief Implements a Qwixx scorepad.
 * @details Qwixx scorepads consist of four rows colored red, yellow, green, and blue from top to bottom.
 * Each row has 11 spaces that can be marked and consists of the numbers 2 through 12. The numbers are
 * in increasing order for the red and yellow rows and in decreasing order for the green and blue rows.
 * The final space (12 for red and yellow, 2 for green and blue) is the lock space. It can only be marked
 * if at least five other spaces to the left of this space have been marked. The final component of the
 * scorepad is the penalty counter. This starts at 0 for each player, and increases by 1 each time a player
 * must mark a penalty.
 */
class Scorepad {
public:
    Scorepad();
    void mark_move(const Move& move);

    /**
     * @brief Increments the internal penalty counter.
     * @return A bool which is true if the penalty counter has reached the
     * maximum number of penalties needed for the game to end, or false otherwise.
     */
    bool mark_penalty() {
        return (++m_penalties >= GameConstants::MAX_PENALTIES);
    };

    /**
     * @brief Gets the index of the rightmost space that has been marked in the given row.
     * @return A size_t option that equals the null option if no spaces have been marked
     * in this row, or otherwise the index of the rightmost space that has been marked.
     */
    std::optional<size_t> get_rightmost_mark_index(Color color) const {
        return m_rightmost_mark_indices[static_cast<size_t>(color)];
    }

    /**
     * @brief Gets the number of marks in the given row.
     * @return An int corresponding to the number of marks that have been placed in the row.
     */
    int get_num_marks(Color color) const {
        return m_num_marks[static_cast<size_t>(color)];
    }

    /**
     * @brief Gets the number of penalties.
     * @return An int corresponding to the number of penalties that have been marked.
     */
    int get_num_penalties() const {
        return m_penalties;
    }

    friend std::ostream& operator<< (std::ostream& stream, const Scorepad& scorepad);

protected:
    /// @brief 2D array of cells. Each cell stores a bool indicating whether it has been marked.
    std::array<std::array<bool, GameConstants::NUM_CELLS_PER_ROW>, GameConstants::NUM_ROWS> m_rows;

    /// @brief Array storing a size_t option for each row indicating the index of the rightmost space that has been marked.
    std::array<std::optional<size_t>, GameConstants::NUM_ROWS> m_rightmost_mark_indices;

    /// @brief Array storing an int for each row indicating the number of marks that have been placed in that row.
    std::array<int, GameConstants::NUM_ROWS> m_num_marks;

    /// @brief The number of penalties that have been marked so far.
    int m_penalties;
};

/**
 * @struct State game.hpp "src/game.hpp"
 * @brief Implements the current state of a running game of Qwixx.
 * @details The complete state is characterized by the set of scorepads
 * and the currently active player. In order to make accessing certain 
 * information easier, other data members also exist. See the description
 * of each data member below.
 */
struct State {
    /// @brief Vector containing one scorepad for each player in the game.
    std::vector<Scorepad> scorepads;

    /// @brief Bitset indicating which rows have been locked. Cleared after each action.
    std::bitset<GameConstants::NUM_ROWS> locks;

    /// @brief Array of bools, where each bool indicates whether the corresponding row has been locked.
    std::array<bool, GameConstants::NUM_ROWS> locked_rows;

    /// @brief size_t indicating which player is currently active.
    size_t curr_player;

    /// @brief int representing the turn count.
    int turn_count;

    /// @brief int representing how many locks have been marked so far.
    int num_locks;

    /// @brief bool indicating whether we are in a terminal state.
    bool is_terminal;

    /// @brief Default comstructor.
    /// @param num_players A size_t representing the number of players in this game.
    /// @param starting_player A size_t representing the starting player.
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

/**
 * @struct GameData game.hpp "src/game.hpp"
 * @brief Holds data about the game that may be useful for collecting statistics.
 * @details Keeps track of which player(s) won, what the final score was, what the
 * final state was, the evaluation history with respect to player 0 (only valid for
 * 2-player games), and the number of turns.
 */
struct GameData {
    std::vector<size_t> winners;
    std::vector<int> final_score;
    std::unique_ptr<State> final_state;
    std::vector<double> p0_evaluation_history;
    int num_turns;
};

/**
 * @class Game game.hpp src/game.hpp
 * @brief Implements a Qwixx game.
 * @details The Game class is responsible for setting up a new game
 * to run by constructing a State object, which will result in a Scorepad
 * object being constructed for each player in the game. The Game object
 * can then be run using the run() method, which will process the game
 * turn by turn until a terminal state is reached, at which point the
 * game will end and a pointer to a GameData object will be returned to
 * the caller.
 */
class Game {
public:
    Game(std::vector<Agent*> players, bool human_active, bool use_evaluation);
    std::unique_ptr<GameData> run();
    std::vector<int> compute_score() const;
    double evaluate_2p();

protected:
    /// @brief A size_t representing the number of players for this Qwixx game.
    size_t m_num_players;

    /// @brief A unique_ptr to the game state.
    std::unique_ptr<State> m_state;

    /// @brief A vector of pointers to the agents for this Qwixx game.
    std::vector<Agent*> m_players;

    /// @brief A bool indicating whether a human player is active in this game.
    bool m_human_active;

    /// @brief A bool indicating whether the evaluation function should be used.
    bool m_use_evaluation;

    /// @brief An array of ints representing the relative frequency for rolling the number in each space.
    /// @details This variable is used by the evaluation function.
    //                                                         2  3  4  5  6  7  8  9  10 11 12 (or reverse)
    static constexpr std::array<int, 11> m_frequency_counts = {1, 2, 3, 4, 5, 6, 5, 4, 3, 2, 1};

    /// @brief An int representing the maximum number of frequency counts left in a row at the start of the game.
    /// @details This variable is used by the evaluation function.
    static constexpr int m_max_frequency_count_left = GameConstants::NUM_ROWS * std::accumulate(m_frequency_counts.begin(), m_frequency_counts.end(), 0);
    
    double m_score_diff_weight;                 //< Score difference weight. Used by the evaluation function.
    double m_freq_count_diff_weight;            //< Frequency count difference weight. Used by the evaluation function.
    double m_lock_progress_diff_weight;         //< Lock progress difference weight. Used by the evaluation function.

    double m_score_diff_scale_factor;           //< Score difference scale factor. Used by the evaluation function. 
    double m_freq_count_diff_scale_factor;      //< Frequency count difference scale factor. Used by the evaluation function.
    double m_lock_progress_diff_scale_factor;   //< Lock progress difference scale factor. Used by the evaluation function.
    double m_lock_progress_diff_bias;           //< Lock progress difference bias. Used by the evaluation function.

    template <ActionType A, typename F>
    bool resolve_action(MoveContext& ctxt, F lock_added);
};

/**
 * @brief Helper function to translate row indices to values.
 * @param color A color enum representing the color of the space.
 * @param index A size_t representing the index of the space.
 * @return An int representing the value of the space at the given index.
 */
constexpr int index_to_value(Color color, size_t index) {
    if (color == Color::red || color == Color::yellow) {
        return static_cast<int>(index + 2);
    }
    else {
        return static_cast<int>(12 - index);
    }
};

/**
 * @brief Helper function to translate row values to indices.
 * @param color A color enum representing the color of the space.
 * @param index An int representing the value of the space.
 * @return A size_t representing the index of the space with the given value.
 */
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

/**
 * @brief Function to resolve the current game action.
 * @details This template function is instantiated for both action types (first and second).
 * For either action, the same general procedure is followed. First, the span of legal moves
 * is generated. Then each agent who is currently allowed to move is requested for a move.
 * Mark each agent's scorepad as needed, including penalties. Then check if any new locks
 * have been marked. If so, invoke the corresponding callback function. Return whether the active
 * player made a move or not.
 * @attention This template function needs to be defined in the header file so as to avoid
 * needing to instantiate a specific callable type.
 * @param ctxt A reference to the current MoveContext. The legal moves stored here need to be filled in by generate_legal_moves().
 * @param lock_added A callback that should be invoked if any locks were added during this action.
 * @return A bool indicating whether the active player made a move during this action.
 */
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

    // Invoke callback if any new locks were marked
    if (m_state->locks.any()) {
        lock_added();
    }

    // Return whether the active player made a move
    return active_player_made_move;
}

std::ostream& operator<< (std::ostream& stream, const MoveContext& ctxt);