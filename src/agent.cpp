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
    
    std::cout << "Your scorepad (player " << m_position << "):\n" << state.scorepads[m_position] << '\n';
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
    if (first_action) {
        m_made_first_action_move = false;
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
            m_made_first_action_move = true;
        }

        return action_one_choice;
    }
    else {
        assert(state.curr_player == m_position);
        
        std::optional<size_t> action_two_choice = std::nullopt;

        if (m_made_first_action_move) {
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
    static std::array<int, GameConstants::NUM_CELLS_PER_ROW> roll_frequencies = {1, 2, 3, 4, 5, 6, 5, 4, 3, 2, 1};

    if (first_action) {
        m_made_first_action_move = false;
    }

    const int red_marks = state.scorepads[m_position].get_num_marks(Color::red);
    const int yellow_marks = state.scorepads[m_position].get_num_marks(Color::yellow);
    const int green_marks = state.scorepads[m_position].get_num_marks(Color::green);
    const int blue_marks = state.scorepads[m_position].get_num_marks(Color::blue);

    m_top_row_fast = (red_marks >= yellow_marks) ? Color::red : Color::yellow;
    m_bottom_row_fast = (green_marks >= blue_marks) ? Color::green : Color::blue;

    auto get_choice = [&](int penalty_avoidance_skips, std::span<const Move> moves) {
        std::array<std::tuple<int, int, bool, std::optional<size_t>>, GameConstants::NUM_ROWS> candidates;
        candidates.fill(std::tuple(std::numeric_limits<int>::max(), 0, false, std::nullopt));

        for (size_t i = 0; i < moves.size(); ++i) {
            const Color move_color = moves[i].color;
            const size_t move_index = moves[i].index;
            const int num_marks = state.scorepads[m_position].get_num_marks(move_color);

            const std::optional<size_t> rightmost_index = state.scorepads[m_position].get_rightmost_mark_index(move_color);
            size_t skipped_spaces_start = rightmost_index.has_value() ? rightmost_index.value() + 1 : 0;
            int num_skips = 0;
            for (size_t j = skipped_spaces_start; j < move_index; ++j) {
                num_skips += roll_frequencies[j];
            }

            bool num_skips_ok = false;

            if (moves[i].index == GameConstants::LOCK_INDEX || (num_marks + 1) >= GameConstants::MIN_MARKS_FOR_LOCK) {
                num_skips_ok = true;
            }
            else {
                if (move_color == m_top_row_fast || move_color == m_bottom_row_fast) {
                    int num_future_skips = 0;
                    for (size_t j = moves[i].index + 1; j < GameConstants::LOCK_INDEX; ++j) {
                        num_future_skips += roll_frequencies[j];
                    }
                    if (num_future_skips / static_cast<int>(GameConstants::MIN_MARKS_FOR_LOCK - (num_marks + 1)) >= 5) {
                        num_skips_ok = true;
                    }
                    else {
                        num_skips_ok = false;
                    }
                }
                else {
                    num_skips_ok = (num_skips <= 4 + penalty_avoidance_skips);
                }
            }

            if (num_skips_ok && num_skips < std::get<0>(candidates[static_cast<size_t>(move_color)])) {
                candidates[static_cast<size_t>(move_color)] = std::tuple(num_skips, num_marks, move_index == GameConstants::LOCK_INDEX, i);
            }
        }

        std::sort(candidates.begin(), 
                  candidates.end(),
                  [](const std::tuple<int, int, bool, std::optional<size_t>> &a, const std::tuple<int, int, bool, std::optional<size_t>> &b) { 
                        // Important: avoid overflow during the main comparison
                        if (!std::get<3>(a).has_value()) {
                            return false;
                        }
                        if (!std::get<3>(b).has_value()) {
                            return true;
                        }
                        if (std::get<2>(a) != std::get<2>(b)) {
                            return std::get<2>(a);
                        }
                        else {
                            return (3 * (std::get<1>(a) - std::get<1>(b)) - (std::get<0>(a) - std::get<0>(b)) >= 0);
                        }
                    }
                 );

        return std::get<3>(candidates[0]);
    };

    if (first_action) {
        std::optional<size_t> action_one_choice = std::nullopt;
        std::optional<size_t> tentative_action_two_choice = std::nullopt;
        
        action_one_choice = get_choice(0, current_action_legal_moves);
        if (!action_one_choice.has_value() && state.curr_player == m_position) {
            tentative_action_two_choice = get_choice(3, action_two_possible_moves);
            if (!tentative_action_two_choice.has_value()) {
                action_one_choice = get_choice(3, current_action_legal_moves);
            }
        }

        if (action_one_choice.has_value()) {
            m_made_first_action_move = true;
        }

        return action_one_choice;
    }
    else {
        std::optional<size_t> action_two_choice = std::nullopt;
        action_two_choice = get_choice(0, current_action_legal_moves);
        if (!action_two_choice.has_value() && !m_made_first_action_move) {
            action_two_choice = get_choice(3, current_action_legal_moves);
        }

        return action_two_choice;
    }
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

    auto lock_possible = [&](const Move move) {
        const int num_marks = state.scorepads[m_position].get_num_marks(move.color);
        const int spaces_remaining = static_cast<int>((GameConstants::LOCK_INDEX - 1) - move.index);
        return (num_marks + spaces_remaining + 1 >= GameConstants::MIN_MARKS_FOR_LOCK) ? true : false;
    };
    
    auto get_value = [&](const Move move) {
        const Color move_color = move.color;
        const size_t move_index = move.index;
        const std::optional<size_t> rightmost_index = state.scorepads[m_position].get_rightmost_mark_index(move_color);
        const int num_marks = state.scorepads[m_position].get_num_marks(move_color);

        double skipping_penalty = 0;
        size_t skipped_spaces_start = rightmost_index.has_value() ? rightmost_index.value() + 1 : 0;
        for (size_t i = skipped_spaces_start; i < move_index; ++i) {
            skipping_penalty += ((m_basic_values[i].base_penalty + (lock_possible(move) ? 0 : -1)) * m_mu * std::pow(m_delta * m_sigma, m_basic_values[i].roll_frequency)); // static_cast<double>(num_same_space_remaining);
        }

        const double base_value = static_cast<double>(num_marks + 1);
        const double future_value_bonus = static_cast<double>((GameConstants::LOCK_INDEX - 1) - move_index) * 0.5 + (lock_possible(move) ? 0.5 : 0.0);
        const double mark_frequency = static_cast<double>(m_basic_values[move_index].roll_frequency);
        const double score = base_value + future_value_bonus * m_epsilon - (std::pow(m_alpha, mark_frequency) * skipping_penalty);

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

void Computational::mutate(double new_average_score, bool final_mutation) {
    if (m_last_mutation.has_value()) {
        if (new_average_score > m_average_score) {
            m_average_score = new_average_score;
        }
        else {
            double old_value = std::get<1>(m_last_mutation.value());
            switch (std::get<0>(m_last_mutation.value())) {
                case Param::alpha:
                    m_alpha = old_value;
                    break;
                case Param::mu:
                    m_mu = old_value;
                    break;
                case Param::delta:
                    m_delta = old_value;
                    break;
                case Param::sigma:
                    m_sigma = old_value;
                    break;
                case Param::epsilon:
                    m_epsilon = old_value;
                    break;
            }
        }
    }

    std::uniform_int_distribution<int> param_dist(0, 4);
    std::uniform_int_distribution<int> direction_dist(0, 1);
    Param p = Param::alpha;

    const double mutation_factor = (1.0 + (direction_dist(rng()) == 0 ? 0.02 : -0.02));

    switch (param_dist(rng())) {
        case 0:
            p = Param::alpha;
            m_last_mutation = std::tuple(p, m_alpha);
            m_alpha = std::min(1.0, m_alpha * mutation_factor);
            break;
        case 1:
            p = Param::mu;
            m_last_mutation = std::tuple(p, m_mu);
            m_mu = std::min(1.0, m_mu * mutation_factor);
            break;
        case 2:
            p = Param::delta;
            m_last_mutation = std::tuple(p, m_delta);
            m_delta = std::min(1.0, m_delta * mutation_factor);
            break;
        case 3:
            p = Param::sigma;
            m_last_mutation = std::tuple(p, m_sigma);
            m_sigma = std::min(1.0, m_sigma * mutation_factor);
            break;
        case 4:
            p = Param::epsilon;
            m_last_mutation = std::tuple(p, m_epsilon);
            m_epsilon = std::min(1.0, m_epsilon * mutation_factor);
            break;
    }

    if (final_mutation) {
        std::cout << "New mutation values:\n" 
                << "alpha = " << m_alpha << '\n'
                << "mu = " << m_mu << '\n'
                << "delta = " << m_delta << '\n'
                << "sigma = " << m_sigma << '\n'
                << "epsilon = " << m_epsilon << '\n'
                << "Average score = " << m_average_score << "\n\n";
    }

}