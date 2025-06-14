#include "agent.hpp"
#include "game.hpp"
#include "rng.hpp"

#include <cassert>
#include <cmath>

std::optional<size_t> Human::make_move(bool first_action, std::span<const Move> current_action_legal_moves, std::span<const Move> action_two_possible_moves, const State& state) {
    (void) first_action, (void) action_two_possible_moves, (void) state;
    for (size_t i = 0; i < state.scorepads.size(); ++i) {
        if (i == m_position) {
            continue;
        }

        std::cout << "\nPlayer " << i << "\'s scorepad:\n" << state.scorepads[i] << '\n';
    }
    
    std::cout << "Your scorepad:\n" << state.scorepads[m_position] << '\n';
    std::string move_string = "";
    for (size_t i = 0; i < current_action_legal_moves.size(); ++i) {
       move_string += (std::to_string(i+1) + ": " + color_to_string[current_action_legal_moves[i].color]
                   + ' ' + std::to_string(index_to_value(current_action_legal_moves[i].color, current_action_legal_moves[i].index)) + '\n');  
    }
    

    if (state.curr_player == m_position) {
        std::cout << "YOU ARE THE ACTIVE PLAYER. ";
    }
    
    std::cout << "The available moves are:\n" << move_string
              << "Please type the number of your chosen move, or type 0 to pass.\n";
    size_t choice = 0;
    std::cin >> choice;
    while (std::cin.fail() || choice > current_action_legal_moves.size()) {
        std::cin.clear();
        std::cin.ignore(std::numeric_limits<std::streamsize>::max(), '\n');
        std::cin >> choice;
    }

    return choice == 0 ? std::nullopt : std::optional<size_t>(choice - 1);
}


// TODO: double check how const works for containers
std::optional<size_t> Random::make_move(bool first_action, std::span<const Move> current_action_legal_moves, std::span<const Move> action_two_possible_moves, const State& state) {        
    (void) first_action, (void) action_two_possible_moves, (void) state;
    std::uniform_int_distribution<size_t> dist(0, current_action_legal_moves.size());
    const size_t move_index = dist(rng());
    return move_index == 0 ? std::nullopt : std::optional<size_t>(move_index - 1);
};

std::optional<size_t> Greedy::make_move(bool first_action, std::span<const Move> current_action_legal_moves, std::span<const Move> action_two_possible_moves, const State& state) {
    (void) first_action, (void) action_two_possible_moves;
    
    std::optional<size_t> choice = std::nullopt;
    int fewest_skips_seen = std::numeric_limits<int>::max();
    for (size_t i = 0; i < current_action_legal_moves.size(); ++i) {
        const Color move_color = current_action_legal_moves[i].color;
        const size_t move_index = current_action_legal_moves[i].index;
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

std::optional<size_t> GreedyImproved::make_move(bool first_action, std::span<const Move> current_action_legal_moves, std::span<const Move> action_two_possible_moves, const State& state) {
    static bool made_first_action_move = false;
    if (first_action) {
        made_first_action_move = false;
    }

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

        if (action_one_choice.has_value()) {
            made_first_action_move = true;
        }

        return action_one_choice;
    }
    else {
        assert(state.curr_player == m_position);
        
        std::optional<size_t> action_two_choice = std::nullopt;

        if (made_first_action_move) {
            action_two_choice = get_choice(m_standard_max_skips, current_action_legal_moves);
        }
        else {
            // Add leniency to avoid penalty
            action_two_choice = get_choice(m_standard_max_skips + m_standard_max_penalty_avoidance_skips, current_action_legal_moves);
        }

        return action_two_choice;
    }
}

std::optional<size_t> RushLocks::make_move(bool first_action, std::span<const Move> current_action_legal_moves, std::span<const Move> action_two_possible_moves, const State& state) {    
    (void) first_action, (void) action_two_possible_moves;
    
    std::optional<size_t> choice = std::nullopt;
    
    for (size_t i = 0; i < current_action_legal_moves.size(); ++i) {
        // Always mark lock if possible
        if (current_action_legal_moves[i].index == GameConstants::LOCK_INDEX) {
            return i;
        }
    }

    int most_marks_seen = std::numeric_limits<int>::min();
    for (size_t i = 0; i < current_action_legal_moves.size(); ++i) {
        const Color move_color = current_action_legal_moves[i].color;
        const size_t move_index = current_action_legal_moves[i].index;
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
    for (size_t i = 0; i < current_action_legal_moves.size(); ++i) {
        const Color move_color = current_action_legal_moves[i].color;
        const size_t move_index = current_action_legal_moves[i].index;
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

Computational::Computational() : Agent(), m_basic_values{} {
    // TODO: double check this initialization method
    int penalty = 12;
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

    struct MoveValue {
        size_t index;
        double value;
    };

    auto get_value = [&](const Move move) {
        const Color move_color = move.color;
        const size_t move_index = move.index;
        const std::optional<size_t> rightmost_index = state.scorepads[m_position].get_rightmost_mark_index(move_color);
        const int num_marks = state.scorepads[m_position].get_num_marks(move_color);

        double skipping_penalty = 0;
        size_t skipped_spaces_start = rightmost_index.has_value() ? rightmost_index.value() + 1 : 0;
        for (size_t i = skipped_spaces_start; i < move_index; ++i) {
            /*int num_same_space_remaining = 0;
            for (size_t j = 0; j < GameConstants::NUM_ROWS; ++j) {
                const Color row_color = static_cast<Color>(j);
                const std::optional<size_t> row_rightmost_index = state.scorepads[m_position].get_rightmost_mark_index(row_color);
                if (!row_rightmost_index.has_value()) {
                    ++num_same_space_remaining;
                    continue;
                }
                else {
                    if (row_rightmost_index.value() < i) {
                        ++num_same_space_remaining;
                    }
                }
            }

            assert(num_same_space_remaining >= 1);*/

            skipping_penalty += (m_basic_values[i].base_penalty * m_mu * std::pow(m_delta * m_sigma, m_basic_values[i].roll_frequency)); /// static_cast<double>(num_same_space_remaining);
        }

        const double base_value = static_cast<double>(num_marks + 1);
        const double mark_frequency = static_cast<double>(m_basic_values[move_index].roll_frequency);
        const double score = base_value - (std::pow(m_alpha, mark_frequency) * skipping_penalty);

        //std::cout << color_to_string[move_color] + ' ' + std::to_string(index_to_value(move_color, move_index)) + " score: " + std::to_string(score) + '\n';

        return score;
    };

    assert(current_action_legal_moves.size() > 0);
    
    std::vector<MoveValue> current_action_scores{};
    std::vector<MoveValue> action_two_scores{};

    if (first_action) {
        std::optional<size_t> action_one_choice = std::nullopt;
        
        for (size_t i = 0; i < current_action_legal_moves.size(); ++i) {
            current_action_scores.push_back({i, get_value(current_action_legal_moves[i])});
        }
        
        std::sort(current_action_scores.begin(), 
                  current_action_scores.end(),
                  [](const MoveValue &a, const MoveValue &b) { return a.value > b.value; }
                 );

        if (current_action_scores[0].value > 0) {
            action_one_choice = current_action_scores[0].index;
        }
        else {
            for (size_t i = 0; i < action_two_possible_moves.size(); ++i) {
                action_two_scores.push_back({i, get_value(action_two_possible_moves[i])});
            }
        
            std::sort(action_two_scores.begin(), 
                      action_two_scores.end(),
                      [](const MoveValue &a, const MoveValue &b) { return a.value > b.value; }
                     );

            if (action_two_scores.size() > 0 && action_two_scores[0].value > 0) {
                action_one_choice = std::nullopt;
            }
            else {
                if (action_two_scores.size() == 0 || current_action_scores[0].value > action_two_scores[0].value) {
                    if (current_action_scores[0].value + static_cast<double>(GameConstants::PENALTY_VALUE) > 0) {
                        action_one_choice = current_action_scores[0].index;
                    }
                    else {
                        action_one_choice = std::nullopt;
                    }
                }
            }
        }
        
        if (action_one_choice.has_value()) {
            m_made_first_action_move = true;
        }

        return action_one_choice;
    }
    else {
        std::optional<size_t> action_two_choice = std::nullopt;
        
        for (size_t i = 0; i < current_action_legal_moves.size(); ++i) {
            current_action_scores.push_back({i, get_value(current_action_legal_moves[i])});
        }
        
        std::sort(current_action_scores.begin(), 
                  current_action_scores.end(),
                  [](const MoveValue &a, const MoveValue &b) { return a.value > b.value; }
                 );

        double penalty_avoidance = m_made_first_action_move ? 0 : -5;
        if (current_action_scores[0].value > 0 + penalty_avoidance) {
            action_two_choice = current_action_scores[0].index;
        }

        return action_two_choice;
    }
}