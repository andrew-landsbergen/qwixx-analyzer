#include "agent.hpp"
#include "game.hpp"
#include "rng.hpp"

#include <cassert>
#include <cmath>

/**
 * @brief The function implementing the human policy for making moves.
 * @details Displays the list of legal moves and requests input from the user
 * corresponding to which move they would like to pick, or 0 if they want to pass.
 * Also see the other documentation for make_move() in the Agent base class (src/agent.hpp).
 */
std::optional<size_t> Human::make_move(bool first_action, std::span<const Move> current_action_legal_moves, std::span<const Move> action_two_possible_moves, const State& state) {
    // Discard unused parameters
    (void) first_action, (void) action_two_possible_moves, (void) state;

    // Display the scorepads of the other players
    for (size_t i = 0; i < state.scorepads.size(); ++i) {
        if (i == m_position) {
            continue;
        }

        std::cout << "\nPlayer " << i << "\'s scorepad:\n" << state.scorepads[i] << '\n';
    }
    
    // Display the human's scorepad
    std::cout << "Your scorepad (player " << m_position << "):\n" << state.scorepads[m_position] << '\n';

    // Generate the string displaying all of the legal moves
    std::string move_string = "";
    for (size_t i = 0; i < current_action_legal_moves.size(); ++i) {
       move_string += (std::to_string(i+1) + ": " + color_to_string[current_action_legal_moves[i].color]
                   + ' ' + std::to_string(index_to_value(current_action_legal_moves[i].color, current_action_legal_moves[i].index)) + '\n');  
    }
    
    // Indicate whether the player is the currently active player
    if (state.curr_player == m_position) {
        std::cout << "YOU ARE THE ACTIVE PLAYER. ";
    }
    
    // Print the list of moves
    std::cout << "The available moves are:\n" << move_string
              << "Please type the number of your chosen move, or type 0 to pass.\n";

    // Get the player's choice of move, retrying until a valid input is typed
    size_t choice = 0;
    std::cin >> choice;
    while (std::cin.fail() || choice > current_action_legal_moves.size()) {
        std::cin.clear();
        std::cin.ignore(std::numeric_limits<std::streamsize>::max(), '\n');
        std::cin >> choice;
    }

    // Return the choice
    return choice == 0 ? std::nullopt : std::optional<size_t>(choice - 1);
}

/**
 * @brief The function implementing the random policy for making moves.
 * @details Chooses a move at random. Passing is considered a move for this purpose,
 * and is weighted once.
 * Also see the other documentation for make_move() in the Agent base class (src/agent.hpp).
 */
std::optional<size_t> Random::make_move(bool first_action, std::span<const Move> current_action_legal_moves, std::span<const Move> action_two_possible_moves, const State& state) {        
    // Discard unused parameters
    (void) first_action, (void) action_two_possible_moves, (void) state;

    // Make uniform random choice
    std::uniform_int_distribution<size_t> dist(0, current_action_legal_moves.size());
    const size_t move_index = dist(rng());

    // Return the choice
    return move_index == 0 ? std::nullopt : std::optional<size_t>(move_index - 1);
};

/**
 * @brief The function implementing the greedy policy for making moves.
 * @details Chooses the move that minimizes the number of skipped spaces, choosing to pass if
 * no move with at most m_max_skips can be found. Ties are broken in order of color from top
 * to bottom of the scorepad.
 * Also see the other documentation for make_move() in the Agent base class (src/agent.hpp).
 */
std::optional<size_t> Greedy::make_move(bool first_action, std::span<const Move> current_action_legal_moves, std::span<const Move> action_two_possible_moves, const State& state) {
    // Discard unused parameters
    (void) first_action, (void) action_two_possible_moves;
    
    std::optional<size_t> choice = std::nullopt;
    int fewest_skips_seen = std::numeric_limits<int>::max();
    for (size_t i = 0; i < current_action_legal_moves.size(); ++i) {
        // Get the current move color, index, and the rightmost index for the current row
        const Color move_color = current_action_legal_moves[i].color;
        const size_t move_index = current_action_legal_moves[i].index;
        const std::optional<size_t> rightmost_index = state.scorepads[m_position].get_rightmost_mark_index(move_color);

        // Default if there is no rightmost index
        int num_skips = static_cast<int>(move_index);
        if (rightmost_index.has_value()) {
            // Invariant: move_index > rightmost_index, so num_skips >= 0
            num_skips = static_cast<int>(move_index - rightmost_index.value()) - 1;
        }

        // Check if move is eligible and better than other moves seen so far
        if (num_skips <= m_max_skips && num_skips < fewest_skips_seen) {
            // Set this move as the new choice and this number of skips as the new fewest number of skips
            choice = i;
            fewest_skips_seen = num_skips;
        }
    }

    // Return the choice
    return choice;
}

/**
 * @brief The function implementing the improved greedy policy for making moves.
 * @details Chooses the move that minimizes the number of skipped spaces, choosing to pass if
 * no move with at most m_max_skips can be found. The effective maximum number of skips is
 * increased if a penalty would otherwise be incurred. Ties are broken in order of color from top
 * to bottom of the scorepad.
 * Also see the other documentation for make_move() in the Agent base class (src/agent.hpp).
 */
std::optional<size_t> GreedyImproved::make_move(bool first_action, std::span<const Move> current_action_legal_moves, std::span<const Move> action_two_possible_moves, const State& state) {
    if (first_action) {
        m_made_first_action_move = false;
    }

    // Lambda to get choice according to the greedy approach, passing in the maximum number of skips allowed
    auto get_choice = [&](int max_skips, std::span<const Move> moves) {
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
            if (num_skips <= max_skips && num_skips < fewest_skips_seen) {
                choice = i;
                fewest_skips_seen = num_skips;
            }
        }
        return choice;
    };
    
    if (first_action) {
        std::optional<size_t> action_one_choice = std::nullopt;
        std::optional<size_t> tentative_action_two_choice = std::nullopt;
        action_one_choice = get_choice(m_standard_max_skips, current_action_legal_moves);
        
        // Currently about to pass as the active player, so check our potential action two move
        if (!action_one_choice.has_value() && state.curr_player == m_position) {
            tentative_action_two_choice = get_choice(m_standard_max_skips, action_two_possible_moves);
            
            // Try to make a choice again, with more leniency this time (may still pass)
            if (!tentative_action_two_choice.has_value()) {
                action_one_choice = get_choice(m_standard_max_skips + m_standard_max_penalty_avoidance_skips, current_action_legal_moves);
            }
        }

        // Remember that we made a move
        if (action_one_choice.has_value()) {
            m_made_first_action_move = true;
        }

        // Return the choice
        return action_one_choice;
    }
    else {        
        std::optional<size_t> action_two_choice = std::nullopt;

        if (m_made_first_action_move) {
            // We won't take a penalty, so use the standard maximum
            action_two_choice = get_choice(m_standard_max_skips, current_action_legal_moves);
        }
        else {
            // We will take a penalty if we don't pass, so increase the maximum
            action_two_choice = get_choice(m_standard_max_skips + m_standard_max_penalty_avoidance_skips, current_action_legal_moves);
        }

        // Return the choice
        return action_two_choice;
    }
}

/**
 * @brief The function implementing the rush policy for making moves.
 * @details The basic idea is to try to make a lock available as quickly as possible in both
 * the top section (red and yellow rows) and the bottom section (green and blue rows). The rows
 * where progress towards the locks is prioritized are called fast rows. By quickly marking the
 * locks in both fast rows, the game will be guaranteed to end, meaning that filling out the slow
 * rows is less important. The choice of fast row can change however, dependent on which row in
 * each section has made the most progress in terms of the number of marks. The reason two fast
 * rows are used, one for each section, is that the number required to mark a top lock is different
 * from the number required to mark a bottom lock, so by having locks available in both sections,
 * the likelihood of rolling a lock on a given turn increases.
 * Also see the other documentation for make_move() in the Agent base class (src/agent.hpp).
 */
std::optional<size_t> RushLocks::make_move(bool first_action, std::span<const Move> current_action_legal_moves, std::span<const Move> action_two_possible_moves, const State& state) {    
    // Relative roll frequencies for each space
    static std::array<int, GameConstants::NUM_CELLS_PER_ROW> roll_frequencies = {1, 2, 3, 4, 5, 6, 5, 4, 3, 2, 1};

    if (first_action) {
        m_made_first_action_move = false;
    }

    const int red_marks = state.scorepads[m_position].get_num_marks(Color::red);
    const int yellow_marks = state.scorepads[m_position].get_num_marks(Color::yellow);
    const int green_marks = state.scorepads[m_position].get_num_marks(Color::green);
    const int blue_marks = state.scorepads[m_position].get_num_marks(Color::blue);

    // Determine which rows are fast
    m_top_row_fast = (red_marks >= yellow_marks) ? Color::red : Color::yellow;
    m_bottom_row_fast = (green_marks >= blue_marks) ? Color::green : Color::blue;

    // Lambda to get choice according to the rush approach
    auto get_choice = [&](int penalty_avoidance_skips, std::span<const Move> moves) {
        // Create an array of tuples representing candidate moves
        // First tuple element is the number of spaces skipped with this move
        // Second tuple element is the number of marks in the row of this move
        // Third tuple element indicates whether this move marks a lock
        // Fourth tuple element is the index of the move into the moves span, or null if there is no valid move
        std::array<std::tuple<int, int, bool, std::optional<size_t>>, GameConstants::NUM_ROWS> candidates;
        candidates.fill(std::tuple(std::numeric_limits<int>::max(), 0, false, std::nullopt));

        for (size_t i = 0; i < moves.size(); ++i) {
            const Color move_color = moves[i].color;
            const size_t move_index = moves[i].index;
            const int num_marks = state.scorepads[m_position].get_num_marks(move_color);

            // Calculate the number of skips in terms of roll frequencies
            // e.g., skipping spaces 2 and 3 to mark the 4 would be 1 + 2 = 3 skips
            const std::optional<size_t> rightmost_index = state.scorepads[m_position].get_rightmost_mark_index(move_color);
            size_t skipped_spaces_start = rightmost_index.has_value() ? rightmost_index.value() + 1 : 0;
            int num_skips = 0;
            for (size_t j = skipped_spaces_start; j < move_index; ++j) {
                num_skips += roll_frequencies[j];
            }

            // Flag to check if we aren't skipping too far ahead
            // Note that skipping far ahead can be counterproductive for a rush strategy, since it
            // decreases the number of options available for future moves in the row
            bool num_skips_ok = false;

            // If we're marking a lock or are already at the number of marks needed to mark the lock in a future move,
            // this move is OK
            if (moves[i].index == GameConstants::LOCK_INDEX || (num_marks + 1) >= GameConstants::MIN_MARKS_FOR_LOCK) {
                num_skips_ok = true;
            }
            else {
                if (move_color == m_top_row_fast || move_color == m_bottom_row_fast) {
                    // Determine how many skips we'll have available after this move
                    int num_future_skips = 0;
                    for (size_t j = moves[i].index + 1; j < GameConstants::LOCK_INDEX; ++j) {
                        num_future_skips += roll_frequencies[j];
                    }

                    // If the number of future skips divided by the number of marks we'll still need to reach the
                    // minimum necessary for the lock is at least 5, this move is OK.
                    // Note that the value 5 is chosen somewhat arbitrarily. For an empty row (no marks yet) with a
                    // minimum of 5 marks needed to gain access to the lock, there are 35 skips remaining and 5 extra
                    // marks needed, for an average of 35 / 5 = 7. We'd like to stay around this average as we progress,
                    // but we may not always have ideal moves available, so we lower the threshold for consideration to
                    // 5 so that reasonable progress can still be made.
                    if (num_future_skips / static_cast<int>(GameConstants::MIN_MARKS_FOR_LOCK - (num_marks + 1)) >= 5) {
                        num_skips_ok = true;
                    }
                    else {
                        num_skips_ok = false;
                    }
                }
                else {
                    // For slow rows, just look at the number of skips
                    num_skips_ok = (num_skips <= 4 + penalty_avoidance_skips);
                }
            }

            // Add the best move of this color to the candidate list
            if (num_skips_ok && num_skips < std::get<0>(candidates[static_cast<size_t>(move_color)])) {
                candidates[static_cast<size_t>(move_color)] = std::tuple(num_skips, num_marks, move_index == GameConstants::LOCK_INDEX, i);
            }
        }

        // Sort the candidate list
        std::sort(candidates.begin(), 
                  candidates.end(),
                  [](const std::tuple<int, int, bool, std::optional<size_t>> &a, const std::tuple<int, int, bool, std::optional<size_t>> &b) { 
                        // Move invalid moves to the back of the list.
                        // Removing this will lead to unexpected results during the main comparison,
                        // due to overflow occuring.
                        if (!std::get<3>(a).has_value()) {
                            return false;
                        }
                        if (!std::get<3>(b).has_value()) {
                            return true;
                        }

                        // If one move marks a lock while another does not, the move marking a lock is sorted ahead of the other move
                        if (std::get<2>(a) != std::get<2>(b)) {
                            return std::get<2>(a);
                        }
                        else {
                            // Compare moves based on three times the difference between their number of marks and the
                            // difference between the number of skips already in the corresponding rows.
                            // For example, if move A has a skip count of 5 and 2 marks have been placed in move A's row,
                            // and move B has a skip count of 3 and 1 mark has been placed in move B's row, then we check
                            // if 3 * (2 - 1) - (5 - 3) = 3 * 1 - 2 = 1 >= 0. This is true, so move A would be sorted ahead
                            // of move B.
                            return (3 * (std::get<1>(a) - std::get<1>(b)) - (std::get<0>(a) - std::get<0>(b)) >= 0);
                        }
                    }
                 );

        // Return the first element in the sorted candidate list
        return std::get<3>(candidates[0]);
    };

    if (first_action) {
        std::optional<size_t> action_one_choice = std::nullopt;
        std::optional<size_t> tentative_action_two_choice = std::nullopt;
        
        action_one_choice = get_choice(0, current_action_legal_moves);

        // If the choice is to pass and we are the active player, look ahead at our tentative second action choice
        if (!action_one_choice.has_value() && state.curr_player == m_position) {
            tentative_action_two_choice = get_choice(3, action_two_possible_moves);
            
            // If we would pass during the second action, make the first action choice with a greater degree of leniency
            if (!tentative_action_two_choice.has_value()) {
                action_one_choice = get_choice(3, current_action_legal_moves);
            }
        }

        // Remember if we made a move during the first action
        if (action_one_choice.has_value()) {
            m_made_first_action_move = true;
        }

        // Return the choice
        return action_one_choice;
    }
    else {
        std::optional<size_t> action_two_choice = std::nullopt;
        action_two_choice = get_choice(0, current_action_legal_moves);

        // If we are going to pass and didn't make a move during the first action,
        // make the choice again with a greater degree of leniency
        if (!action_two_choice.has_value() && !m_made_first_action_move) {
            action_two_choice = get_choice(3, current_action_legal_moves);
        }

        // Return the choice
        return action_two_choice;
    }
}

/**
 * @brief Constructor for the computational agent.
 * @details Calls the base class constructor, then initializes m_basic_values with penalty and frequency values.
 */
Computational::Computational() : Agent(), m_basic_values{} {
    // The base penalty for the leftmost space is 12, since that would be the value of this space
    // if this space plus every space to its right were marked
    int penalty = 12;

    // The frequency value corresponds to the exponent that (delta * sigma) is raised to. Since
    // delta and sigma are discount factors, the effect is that the penalty is discounted more
    // (i.e., is weaker) for spaces that have a low frequency to roll compared to spaces that have
    // a high frequency to roll. For the 2 and 12 spaces, (delta * sigma) is raised to the 5th power.
    // For the 7 space, (delta * sigma) is raised to the 0th power, meaning that this factor will equal
    // 1 and no discount will be applied.
    int frequency = 5;
    for (size_t i = 0; i < m_basic_values.size(); ++i) {
        m_basic_values[i].base_penalty = penalty;
        m_basic_values[i].roll_frequency = frequency;
        --penalty;
        if (i <= 5) {
            --frequency;
        }
        else {
            ++frequency;
        }
    }
}

std::optional<size_t> Computational::make_move(bool first_action, std::span<const Move> current_action_legal_moves, std::span<const Move> action_two_possible_moves, const State& state) {
    if (first_action) {
        m_made_first_action_move = false;
    }

    // We will use this struct to assign a value to each move index
    struct MoveValue {
        size_t index;
        double value;
    };

    // Lambda to check if it is still possible to mark the lock of the row this move is for
    auto lock_possible = [&](const Move move) {
        const int num_marks = state.scorepads[m_position].get_num_marks(move.color);
        const int spaces_remaining = static_cast<int>((GameConstants::LOCK_INDEX - 1) - move.index);
        return (num_marks + spaces_remaining + 1 >= GameConstants::MIN_MARKS_FOR_LOCK) ? true : false;
    };
    
    // Lambda to compute the value of a move
    auto get_value = [&](const Move move) {
        const Color move_color = move.color;
        const size_t move_index = move.index;
        const std::optional<size_t> rightmost_index = state.scorepads[m_position].get_rightmost_mark_index(move_color);
        const int num_marks = state.scorepads[m_position].get_num_marks(move_color);

        // Calculate the skipping penalty associated with the move
        double skipping_penalty = 0;
        size_t skipped_spaces_start = rightmost_index.has_value() ? rightmost_index.value() + 1 : 0;
        for (size_t i = skipped_spaces_start; i < move_index; ++i) {
            // We sum up the penalties associated with each skipped space.
            // The penalty for skipping a space is defined by its base penalty (minus one if the
            // lock in this move's row is no longer possible), multiplied by mu, a factor that reduces
            // the penalty under the assumption that not all spaces to the right of the skipped space
            // will end up marked, and further multiplied by (delta * sigma) raised to the power of
            // the frequency of rolling the number for this move in the future. Delta and sigma are
            // discount factors that reduce the penalty for spaces that are difficult to roll, since
            // skipping a common number like 7 is more significant than skipping a rare number like 2.
            
            // The net result of the above logic is that skipping a 2 and 3 to mark a 4 will be penalized
            // significantly less than skipping a 6 and a 7 to mark an 8, even though the base penalty of
            // the 2 and 3 spaces are higher than the 6 and 7 spaces (since more points can be earned by
            // marking the entire row after the 2 and 3 than after the 6 and 7).
            skipping_penalty += ((m_basic_values[i].base_penalty + (lock_possible(move) ? 0 : -1)) * m_mu * std::pow(m_delta * m_sigma, m_basic_values[i].roll_frequency)); // static_cast<double>(num_same_space_remaining);
        }

        // The base value is the number of points earned by making this move right now, which is equal to the number of marks after making the move
        const double base_value = static_cast<double>(num_marks + 1);

        // The future value is the number of points that this move might be worth in the future.
        // Every mark added to a row effectively increases the average value of each mark in the row by 0.5,
        // so the maximum bonus is 0.5 multiplied by the number of spaces left to mark.
        const double future_value_bonus = static_cast<double>((GameConstants::LOCK_INDEX - 1) - move_index) * 0.5 + (lock_possible(move) ? 0.5 : 0.0);
        
        // Get the frequency of the space being marked itself
        const double mark_frequency = static_cast<double>(m_basic_values[move_index].roll_frequency);

        // The total score is the sum B + F - P.
        // B is just the base value computed above.
        // F is the future value bonus discounted by the efficiency factor epsilon, which reflects the fact
        // that not the entire scorepad will end up getting marked by the end of the game.
        // P is the total skipping penalty as computed above, also discounted slightly if the current move
        // is difficult to roll.
        const double score = base_value + future_value_bonus * m_epsilon - (std::pow(m_alpha, mark_frequency) * skipping_penalty);

        // Return the score
        return score;
    };
    
    // Keep track of scores for both the current action and for the second action
    std::vector<MoveValue> current_action_scores{};
    std::vector<MoveValue> action_two_scores{};

    if (first_action) {
        std::optional<size_t> action_one_choice = std::nullopt;
        
        // Assign scores for all current legal moves
        for (size_t i = 0; i < current_action_legal_moves.size(); ++i) {
            current_action_scores.push_back({i, get_value(current_action_legal_moves[i])});
        }
        
        // Sort the moves by score
        std::sort(current_action_scores.begin(), 
                  current_action_scores.end(),
                  [](const MoveValue &a, const MoveValue &b) { return a.value > b.value; }
                 );

        // If no move has a score greater than the neutral passing score of 0, opt to pass instead
        if (current_action_scores[0].value > 0) {
            action_one_choice = current_action_scores[0].index;
        }

        // Go ahead and return our choice if we aren't the active player
        if (state.curr_player != m_position) {
            return action_one_choice;
        }

        // If we reach here, then we are the active player, so we need to be mindful of the possibility of taking a penalty
        if (current_action_scores[0].value <= 0) {
            // If we do choose to pass, we need to check the action two moves to see if we would
            // end up passing during that action as well
            for (size_t i = 0; i < action_two_possible_moves.size(); ++i) {
                action_two_scores.push_back({i, get_value(action_two_possible_moves[i])});
            }
        
            std::sort(action_two_scores.begin(), 
                      action_two_scores.end(),
                      [](const MoveValue &a, const MoveValue &b) { return a.value > b.value; }
                     );

            // If we won't end up passing during the second action, go ahead and pass now
            if (action_two_scores.size() > 0 && action_two_scores[0].value > 0) {
                action_one_choice = std::nullopt;
            }
            else {
                // If we don't have an action two move or if our best first action move is better than our best
                // second action move, then we need to lower our threshold for passing during this action.
                // A penalty is worth -5 points, so if a move is worth more than -5, we should still take it.
                if (action_two_scores.size() == 0 || current_action_scores[0].value > action_two_scores[0].value) {
                    if (current_action_scores[0].value + static_cast<double>(GameConstants::PENALTY_VALUE) > 0) {
                        action_one_choice = current_action_scores[0].index;
                    }
                    else {
                        // We have a better move during the second action, so just pass during this action
                        action_one_choice = std::nullopt;
                    }
                }
            }
        }
        
        // Remember if we made a move during the first action
        if (action_one_choice.has_value()) {
            m_made_first_action_move = true;
        }

        // Return the choice
        return action_one_choice;
    }
    else {
        std::optional<size_t> action_two_choice = std::nullopt;
        
        // Assign scores for all current legal moves
        for (size_t i = 0; i < current_action_legal_moves.size(); ++i) {
            current_action_scores.push_back({i, get_value(current_action_legal_moves[i])});
        }
        
        // Sort the moves by score
        std::sort(current_action_scores.begin(), 
                  current_action_scores.end(),
                  [](const MoveValue &a, const MoveValue &b) { return a.value > b.value; }
                 );

        // If we passed during the first action, then the passing threshold is reduced by 5, since
        // we will take a penalty if we pass now
        double penalty_avoidance = m_made_first_action_move ? 0 : -GameConstants::PENALTY_VALUE;
        if (current_action_scores[0].value > 0 + penalty_avoidance) {
            action_two_choice = current_action_scores[0].index;
        }

        // Return the choice
        return action_two_choice;
    }
}