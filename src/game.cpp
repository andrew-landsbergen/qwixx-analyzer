#include <algorithm>
#include <array>
#include <chrono>
#include <cstdlib>
#include <ctime>
#include <iostream>
#include <memory>
#include <optional>
#include <random>
#include <span>

#include "agent.hpp"
#include "game.hpp"
#include "rng.hpp"

/**
 * @brief Default constructor.
 * @details Initializes a blank scorepad. No spaces have been marked, so m_rightmost_mark_indices
 * should consist of all null options and m_num_marks should be all 0's. m_penalties should be set
 * to 0 and each flag in m_rows should be set to false.
 */
Scorepad::Scorepad() : m_rightmost_mark_indices{}, m_num_marks{}, m_penalties(0) {
    m_rightmost_mark_indices.fill(std::nullopt);
    
    for (size_t i = 0; i < m_rows.size(); ++i) {
        for (size_t j = 0; j < m_rows[i].size(); ++j) {
            m_rows[i][j] = false;
        }
    }
}

/**
 * @brief Function used to mark a move on the scorepad.
 * @details Sets the flag in m_rows for the given color and index to true,
 * using the color enum as an index. Updates m_rightmost_mark_indices for this
 * color to the new index, since this mark must be to the right of all other
 * marks in this row. Increments the number of marks for this row by 1, and by
 * 1 again if the space marked was a lock (the lock counts as 2 marks for scoring
 * purposes).
 * @attention This function does not check the move passed in to ensure that it
 * is legal and valid. The caller must instead ensure this.
 */
void Scorepad::mark_move(const Move& move) {
    size_t color = static_cast<size_t>(move.color);
    size_t index = move.index;
    m_rows[color][index] = true;
    m_rightmost_mark_indices[color] = index;
    m_num_marks[color] += 1;
    if (index == (GameConstants::LOCK_INDEX)) {
        m_num_marks[color] += 1;
    }
}

/**
 * @brief Function used to roll the game's dice.
 * @details Uses a random number generator to set each element
 * in the rolls span to a number between 1 and 6.
 * @param rolls A span of ints corresponding to the roll values for the game's dice.
 */
void roll_dice(std::span<int> rolls) {
    static std::uniform_int_distribution<int> dist(1, 6);
    for (size_t i = 0; i < rolls.size(); ++i) {
        rolls[i] = dist(rng());
    }
}

/**
 * @brief Function used to generate the set of legal moves for the given action for the given scorepad.
 * @details For each possible move (determined by the available dice and their associated rolls), add this
 * move to the span of legal moves. Memory for the span of legal moves is allocated outside of this function,
 * so this function just needs to return the number of legal moves it found.
 * @attention This function does not perform bounds-checking on the legal_moves span. The caller must ensure that
 * there is enough memory available.
 * @param legal_moves A reference to a span of Move objects. To be updated in this function.
 * @param dice A read-only reference to a span of colors corresponding to the dice that are still remaining in the game.
 * @param rolls A read-only reference to the integer values of the dice rolls. The first two elements are for the white dice.
 * @param scorepad A read-only reference to the Scorepad object of the player we are generating legal moves for.
 * @return A size_t indicating the number of legal moves that were found.
 */
template <ActionType A>
size_t generate_legal_moves(std::span<Move>& legal_moves, const std::span<Color>& dice, const std::span<int>& rolls, const Scorepad& scorepad) {
    size_t num_legal_moves = 0;
    
    auto add_move_if_legal = [&](const Color color, const std::optional<size_t> rightmost_mark_index, const size_t index_to_mark) {
        // Is the number to mark after the rightmost-marked number on the row?
        if (!rightmost_mark_index.has_value() || index_to_mark > rightmost_mark_index.value()) { 
            // Are we marking a lock? If so, have the minimum number of marks been placed to mark the lock?
            if (index_to_mark < (GameConstants::LOCK_INDEX) 
            || ((index_to_mark == GameConstants::LOCK_INDEX) && (scorepad.get_num_marks(color) >= GameConstants::MIN_MARKS_FOR_LOCK)))
            {
                legal_moves[num_legal_moves].color = color;
                legal_moves[num_legal_moves].index = index_to_mark;
                ++num_legal_moves;
            }
        }
    };
    
    int sum_1 = 0;
    int sum_2 = 0;

    // Use dice to get available color rows
    for (size_t i = 2; i < rolls.size(); ++i) {
        if constexpr (A == ActionType::First) {
            sum_1 = rolls[0] + rolls[1];
        }
        else {
            sum_1 = rolls[0] + rolls[i];
            sum_2 = rolls[1] + rolls[i];
        }

        const Color color = dice[i - 2];
        const std::optional<size_t> rightmost_mark_index = scorepad.get_rightmost_mark_index(color);
        const size_t index_to_mark_1 = value_to_index(color, sum_1);
        const size_t index_to_mark_2 = value_to_index(color, sum_2);

        if constexpr (A == ActionType::First) {
            add_move_if_legal(color, rightmost_mark_index, index_to_mark_1);
        }
        else {
            add_move_if_legal(color, rightmost_mark_index, index_to_mark_1);
            add_move_if_legal(color, rightmost_mark_index, index_to_mark_2);
        }
    }

    return num_legal_moves;
}

/**
 * @brief Default constructor.
 * @details Sets the number of players, the vector of pointers to agents, whether
 * a human player is active, and whether to use the evaluation function. Also sets
 * the initial values of the parameters used by the evaluation function. Throws an
 * exception if there are too few or too many players. If the player count is OK,
 * sets the position of each player and randomly selects the starting player, then
 * constructs the State object for this game.
 */
Game::Game(std::vector<Agent*> players, bool human_active, bool use_evaluation) 
    : m_num_players(players.size()), 
      m_players(players), 
      m_human_active(human_active),
      m_use_evaluation(use_evaluation),
      m_score_diff_weight(0.25),
      m_freq_count_diff_weight(0.40),
      m_lock_progress_diff_weight(0.35),
      m_score_diff_scale_factor(20.0),
      m_freq_count_diff_scale_factor(36.0),
      m_lock_progress_diff_scale_factor(2.75),
      m_lock_progress_diff_bias(2.5) {    
    
    if (m_num_players < GameConstants::MIN_PLAYERS || m_num_players > GameConstants::MAX_PLAYERS) {
        throw std::runtime_error("Invalid player count.");
    }

    // Set player positions in the game
    for (size_t i = 0; i < m_players.size(); ++i) {
        m_players[i]->set_position(i);
    }
    
    // Randomly pick starting player
    std::uniform_int_distribution<size_t> dist(0, m_num_players - 1);

    // Construct state
    m_state = std::make_unique<State>(m_num_players, dist(rng()));
}

/**
 * @brief Computes the current score for all players.
 * @details In Qwixx, score is calculated by taking the sum from 1 to the 
 * number of marks in a row for each row, then subtracting the penalty value
 * multiplied by the number of penalties.
 * @return A vector of ints representing the score of each player.
 */
std::vector<int> Game::compute_score() const {
    std::vector<int> scores(m_num_players, 0);

    for (size_t i = 0; i < m_num_players; ++i) {
        int score = 0;
        for (size_t j = 0; j < GameConstants::NUM_ROWS; ++j) {
            int num_marks = m_state.get()->scorepads[i].get_num_marks(static_cast<Color>(j));
            score += (num_marks * (num_marks + 1)) / 2;     // Equivalent to the sum over 1 to num_marks
        }
        score -= GameConstants::PENALTY_VALUE * (m_state.get()->scorepads[i].get_num_penalties());
        scores[i] = score;
    }

    return scores;
}

/**
 * @brief Computes the evaluation with respect to player 0 for a 2-player game of Qwixx.
 * @details The evaluation function compares score difference, space difference, and lock
 * progress difference between the two players. The weights for these features are not
 * static, but change over the course of the game. At the start of the game, space difference
 * is deemed most important, but towards the end of the game, score difference becomes much
 * more important.
 */
double Game::evaluate_2p() {
    // The starting evaluation is 0
    if (m_state->turn_count == 0) {
        return 0.0;
    }

    // On turn 8, start a "ramping" period where the score difference weight increases and
    // the frequency count (space) difference weight and lock progress difference weight
    // decreases. The values of 7 and 22 are somewhat arbitrary. An average Qwixx game between
    // the stronger agents lasts for about 23 turns, so we consider turn 8 to be the end of
    // the early game and turn 23 to be the end of the late game.
    const int ramp_start = 7;
    const int ramp_end = 22;
    const double range = static_cast<double>(ramp_end - ramp_start + 1);
    if (m_state->turn_count >= ramp_start && m_state->turn_count <= ramp_end) {
        m_score_diff_weight += (0.75 - 0.25) / range;
        m_freq_count_diff_weight -= (0.40 - 0.15) / range;
        m_lock_progress_diff_weight -= (0.35 - 0.10) / range;
    }
    
    // Get the current score to compute the score difference term
    std::vector scores = compute_score();
    const int score_diff = scores[0] - scores[1];
    const double score_diff_term = m_score_diff_weight * std::max(-1.0, std::min(1.0, static_cast<double>(score_diff) / m_score_diff_scale_factor));

    // Get the number of frequency counts left for both players
    std::array<int, 2> freq_count_left = {0, 0};
    for (size_t i = 0; i < 2; ++i) {
        for (size_t j = 0; j < GameConstants::NUM_ROWS; ++j) {
            if (m_state->locked_rows[j]) {
                continue;
            }

            const size_t start = m_state->scorepads[i].get_rightmost_mark_index(static_cast<Color>(j)).value_or(0);
            for (size_t k = start; k <= GameConstants::LOCK_INDEX; ++k) {
                freq_count_left[i] += m_frequency_counts[k];
            }
        }
    }

    // Get the difference between frequency counts to compute the frequency count difference term
    const int freq_count_diff = freq_count_left[0] - freq_count_left[1];
    const double freq_count_diff_term = m_freq_count_diff_weight * std::max(-1.0, std::min(1.0, (static_cast<double>(freq_count_diff) / m_freq_count_diff_scale_factor)));

    // Lambda to determine lock progress for the given player and row color
    auto get_lock_progress = [this](size_t player, Color color) {
        // The first value is the number of marks in this row, the second
        // value is the average number of frequency counts left per mark needed
        // in order to gain access to the lock
        std::tuple<int, double> progress = {0, 0.0};

        // If this row is locked, progress is not applicable
        if (m_state->locked_rows[static_cast<size_t>(color)]) {
            return progress;
        }

        const size_t num_marks = static_cast<size_t>(m_state->scorepads[player].get_num_marks(color));
        const size_t rightmost_index = m_state->scorepads[player].get_rightmost_mark_index(color).value_or(0);
        const size_t spaces_left = GameConstants::LOCK_INDEX - rightmost_index + 1;
        const size_t marks_needed = GameConstants::MIN_MARKS_FOR_LOCK - num_marks;

        // It isn't possible to mark the lock in this row, so use the worst values possible for progress
        if (spaces_left < marks_needed) {
            progress = {0, -3.0};
            return progress;
        }

        // It's already possible to mark the lock in this row, so use the best values possible for progress
        if (num_marks >= 5) {
            progress = {5, 3.0};
            return progress;
        }
        
        // Get the number of frequency counts that are still available in this row
        int freq_count_left = 0;
        for (size_t i = rightmost_index + 1; i < GameConstants::LOCK_INDEX; ++i) {
            freq_count_left += m_frequency_counts[i];
        }

        // Compute the average number of frequency counts available for each mark that still needs to be
        // made in order to gain access to the lock
        double freq_count_per_marks_needed = static_cast<double>(freq_count_left) / static_cast<double>(marks_needed);
        
        // Subtract 7 from freq_count_per_marks_needed, since this is equal to the average for an
        // empty row. Also clamp the value between -3 and 3.
        progress = {num_marks, std::max(-3.0, std::min(3.0, freq_count_per_marks_needed - 7.0))};

        return progress;
    };
    
    std::array<double, 2> lock_progress = {0.0, 0.0};

    // For each player, get their best progress in the top and bottom sections
    for (size_t i = 0; i < 2; ++i) {
        const std::tuple<int, double> red_progress = get_lock_progress(i, Color::red);
        const std::tuple<int, double> yellow_progress = get_lock_progress(i, Color::yellow);
        const double top_progress = std::max(
            static_cast<double>(std::get<0>(red_progress)) + std::get<1>(red_progress), 
            static_cast<double>(std::get<0>(yellow_progress)) + std::get<1>(yellow_progress));
        
        const std::tuple<int, double> green_progress = get_lock_progress(i, Color::green);
        const std::tuple<int, double> blue_progress = get_lock_progress(i, Color::blue);
        const double bottom_progress = std::max(
            static_cast<double>(std::get<0>(green_progress)) + std::get<1>(green_progress), 
            static_cast<double>(std::get<1>(blue_progress)) + std::get<1>(blue_progress));

        // Define a player's lock progress as the sum of the top and bottom progress shifted by the bias
        // and all divided by the scale factor, then clamped between -1 and 1
        lock_progress[i] = std::max(-1.0, std::min(1.0, (top_progress + bottom_progress - m_lock_progress_diff_bias) / m_lock_progress_diff_scale_factor));
    }

    // Get the difference between lock progress to compute the lock progress difference term
    const double lock_progress_diff = lock_progress[0] - lock_progress[1];
    const double lock_progress_diff_term = m_lock_progress_diff_weight * std::max(-1.0, std::min(1.0, lock_progress_diff));

    // Return the sum of all terms
    return score_diff_term + freq_count_diff_term + lock_progress_diff_term;
}

/**
 * @brief Runs a game of Qwixx.
 * @details For a newly-constructed Game, the run() method runs the game to completion
 * by alternating between calls to resolve_action() for the first and second action until
 * the game has reached a terminal state. The possible terminal states for Qwixx are
 * a) at least two distinct locks marked and b) one player has at least four penalties.
 * This method is also responsible for removing dice from the games when rows are locked
 * and checking if a player needs to be given a penalty. Once the game is complete,
 * the final score is computed and the winner(s) determined, and a pointer to a GameData
 * object is returned to the caller. If the evaluation function is being used, then
 * evaluate_2p() will be called after each turn and its result stored in an evaluation history
 * to be returned as part of the GameData object.
 * @return A unique pointer to a GameData object containing data about this game of Qwixx.
 */
std::unique_ptr<GameData> Game::run() {        
    // Initial colors of the colored dice. Colored dice may be removed during the game.
    std::vector<Color> dice = { Color::red, Color::yellow, Color::green, Color::blue };

    // Value of dice rolls. The first two represent the white dice. The final four represent the colored dice.
    // Colored dice may be removed during the game.
    std::vector<int> rolls = { 0, 0, 0, 0, 0, 0 };

    // Create containers to be held in MoveContext object
    std::array<Move, GameConstants::MAX_LEGAL_MOVES> current_action_legal_moves{};
    std::array<Move, GameConstants::MAX_LEGAL_MOVES> action_two_possible_moves{};
    std::array<std::optional<Move>, GameConstants::MAX_PLAYERS> registered_moves{};

    // Create initial context
    MoveContext ctxt = {
        std::span<Color>(dice),
        std::span<int>(rolls),
        std::span<Move>(current_action_legal_moves),
        std::span<Move>(action_two_possible_moves),
        std::span<std::optional<Move>>(registered_moves)
    };

    // Lambda to remove the corresponding members from the dice and rolls vectors when a lock has been added
    auto lock_added = [&]() {
        // Check each lock and remove the corresponding dice
        for (size_t i = 0; i < GameConstants::NUM_ROWS; ++i) {
            if (m_state->locks.test(i)) {
                m_state->locked_rows[i] = true;
                Color color_to_remove = static_cast<Color>(i);
                auto it = std::find(dice.begin(), dice.end(), color_to_remove);     // If the value is not found, it is a bug in the program
                dice.erase(it);
                int dist = std::distance(dice.begin(), it);
                rolls.erase(rolls.begin() + (dist + 2));
                ++(m_state->num_locks);
            }
        }

        // Reset the locks so that the next lock addition does not result in num_locks being incremented again for the current locks
        m_state->locks.reset();
    
        // Reconstruct spans for dice and rolls
        ctxt.dice = std::span<Color>(dice);
        ctxt.rolls = std::span<int>(rolls);
    
        // Check number of locks to determine if game has ended
        if (m_state->num_locks >= 2) {
            m_state->is_terminal = true;
        }
    };

    // Lambda to check if a player needs to receive a penalty for passing, and if so,
    // if the game has ended as a result of this new penalty
    auto check_penalties = [this](bool active_player_made_move) {
        if (!active_player_made_move) {
            if (m_state.get()->scorepads[m_state->curr_player].mark_penalty()) {
                m_state->is_terminal = true;
            }
        }
    };

    bool active_player_made_move = false;

    std::vector<double> p0_evaluation_history;
    
    while(!m_state->is_terminal) {                
        // New turn start
        
        // Get evaluation
        if (m_use_evaluation) {
            const double evaluation = evaluate_2p();
            if (m_human_active) {
                std::cout << "Evaluation for player 0: " << evaluation << '\n';
            }
            p0_evaluation_history.push_back(evaluation);
        }

        // Increment turn counter
        m_state->turn_count += 1;
        
        // Roll dice
        roll_dice(ctxt.rolls);

        // Print information about the game state if a human is playing
        if (m_human_active) {
            std::cout << "\nStarting new round.\nRolling dice...\n"
                    << "WHITE: " << ctxt.rolls[0] << ' ' << ctxt.rolls[1] << '\n';

            for (size_t i = 0; i < dice.size(); ++i) {
                std::cout << color_to_string[dice[i]] << ": " << ctxt.rolls[i + 2] << '\n';
            }

            std::cout << "Action one in progress. Player " << m_state->curr_player << " is active.\n";
        }

        // Resolve the first action
        active_player_made_move = resolve_action<ActionType::First>(ctxt, lock_added);

        // Check if the game has ended before starting with the second action
        if (m_state->is_terminal) {
            break;
        }

        // Let the human player know that action two is now starting
        if (m_human_active) {
            std::cout << "Action two in progress. Player " << m_state->curr_player << " is active.\n";
        }

        // Resolve the second action
        active_player_made_move |= resolve_action<ActionType::Second>(ctxt, lock_added);

        // Check if any penalties need to be applied
        check_penalties(active_player_made_move);

        // Reset the flag tracking whether the active player made a move
        active_player_made_move = false;

        // Increment the variable for the current player
        m_state->curr_player = (m_state->curr_player + 1) % m_num_players;
    }

    // Compute the final score for all players
    std::vector<int> final_score = compute_score();

    // Get the max score
    int max_val = *std::max_element(final_score.begin(), final_score.end());
    std::vector<size_t> winners;

    // All players with the max score are deemed winners
    for (size_t i = 0; i < final_score.size(); ++i) {
        if (final_score[i] == max_val) {
            winners.push_back(i);
        }
    }

    // If player 0 won, add a final evaluation of 1, else -1
    p0_evaluation_history.push_back(std::find(winners.begin(), winners.end(), 0) == winners.end() ? -1.0 : 1.0);

    // Construct the GameData object, filling it with the winners, the final score,
    // the final game state, the evalaution history, and the turn count.
    // We need to move all vectors since they were constructed in this function.
    std::unique_ptr<GameData> data = std::make_unique<GameData>(
        std::move(winners),
        std::move(final_score),
        std::move(m_state), 
        std::move(p0_evaluation_history),
        m_state->turn_count);

    // Return the GameData struct
    return data;
}

/**
 * @brief Overload for << operator to allow pretty printing of a Scorepad
 * @return An ostream reference which can be sent to stdout
 */
std::ostream& operator<< (std::ostream& stream, const Scorepad& scorepad) {
    for (size_t i = 0; i < GameConstants::NUM_ROWS; ++i) {
        Color color = static_cast<Color>(i);
        std::string color_str = "";
        switch (color) {
            case Color::red: color_str += "RED       "; break;
            case Color::yellow: color_str += "YELLOW    "; break;
            case Color::green: color_str += "GREEN     "; break;
            case Color::blue: color_str += "BLUE      "; break;
        }
        stream << color_str;
        for (size_t j = 0; j < GameConstants::NUM_CELLS_PER_ROW; ++j) {
            stream << std::setw(4) << (scorepad.m_rows[i][j] ? "X" : std::to_string(index_to_value(color, j)));
        }
        stream << '\n';
    }
    stream << "PENALTIES " << std::setw(4) << scorepad.m_penalties << '\n';
    return stream;
}

/**
 * @brief Overload for << operator to allow pretty printing of a MoveContext
 * @return An ostream reference which can be sent to stdout
 */
std::ostream& operator<< (std::ostream& os, const MoveContext& ctxt) {
    os << "The dice rolls are:\n";
    
    for (size_t i = 0; i < 2; ++i) {
        os << "WHITE: " << ctxt.rolls[i] << '\n';
    }

    for (size_t i = 2; i < ctxt.rolls.size(); ++i) {
        Color color = ctxt.dice[i - 2];
        os << color_to_string[color] << ": " << ctxt.rolls[i] << '\n';
    }

    os << "The legal moves are:\n";

    for (auto move : ctxt.current_action_legal_moves) {
        Color color = move.color;
        int value = index_to_value(color, move.index);
        os << "{ " << color_to_string[color] << ' ' << value << " }, ";
    }

    os << '\n';

    return os;
}

// Instantiations for generate_legal_moves() template (necessary for compilation)
template size_t generate_legal_moves<ActionType::First>(std::span<Move>& legal_moves, const std::span<Color>& dice, const std::span<int>& rolls, const Scorepad& scorepad);
template size_t generate_legal_moves<ActionType::Second>(std::span<Move>& legal_moves, const std::span<Color>& dice, const std::span<int>& rolls, const Scorepad& scorepad);
