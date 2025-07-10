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

Scorepad::Scorepad() : m_rightmost_mark_indices{}, m_num_marks{}, m_penalties(0) {
    m_rightmost_mark_indices.fill(std::nullopt);
    
    for (size_t i = 0; i < m_rows.size(); ++i) {
        for (size_t j = 0; j < m_rows[i].size(); ++j) {
            m_rows[i][j] = false;
        }
    }
}

void Scorepad::mark_move(const Move& move) {
    // TODO: should this validate if the move is legal?
    size_t color = static_cast<size_t>(move.color);
    size_t index = move.index;
    m_rows[color][index] = true;
    m_rightmost_mark_indices[color] = index;
    m_num_marks[color] += 1;
    if (index == (GameConstants::FIRST_LOCK_INDEX)) {
        m_num_marks[color] += 1;
    }
    else if (index == (GameConstants::SECOND_LOCK_INDEX)) {
        m_num_marks[color] += 2;
    }
}

void roll_dice(std::span<int> rolls) {
    static std::uniform_int_distribution<int> dist(1, 6);
    for (size_t i = 0; i < rolls.size(); ++i) {
        rolls[i] = dist(rng());
    }
}

// TODO: review const correctness
template <ActionType A>
size_t generate_legal_moves(std::span<Move>& legal_moves, const std::span<Color>& dice, const std::span<int>& rolls, const Scorepad& scorepad) {
    size_t num_legal_moves = 0;
    
    auto add_move_if_legal = [&](const Color color, const std::optional<size_t> rightmost_mark_index, const size_t index_to_mark) {
        // Is the number to mark after the rightmost-marked number on the row?
        if (!rightmost_mark_index.has_value() || index_to_mark > rightmost_mark_index.value()) { 
            // Are we marking a lock? If so, have the minimum number of marks been placed to mark the lock?
            if (index_to_mark < (GameConstants::FIRST_LOCK_INDEX) 
            || ((index_to_mark == GameConstants::FIRST_LOCK_INDEX) && (scorepad.get_num_marks(color) >= GameConstants::MIN_MARKS_FOR_LOCK_ONE))
            || ((index_to_mark == GameConstants::SECOND_LOCK_INDEX && (scorepad.get_num_marks(color) >= GameConstants::MIN_MARKS_FOR_LOCK_TWO))))
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
    
    std::uniform_int_distribution<size_t> dist(0, m_num_players - 1);
    m_state = std::make_unique<State>(m_num_players, dist(rng()));
}

std::vector<int> Game::compute_score() const {
    std::vector<int> scores(m_num_players, 0);

    for (size_t i = 0; i < m_num_players; ++i) {
        int score = 0;
        for (size_t j = 0; j < GameConstants::NUM_ROWS; ++j) {
            int num_marks = m_state.get()->scorepads[i].get_num_marks(static_cast<Color>(j));
            score += (num_marks * (num_marks + 1)) / 2;
        }
        score -= GameConstants::PENALTY_VALUE * (m_state.get()->scorepads[i].get_num_penalties());
        scores[i] = score;
    }

    return scores;
}

double Game::evaluate_2p() {
    if (m_state->turn_count == 0) {
        return 0.0;
    }

    const int ramp_start = 7;
    const int ramp_end = 22;
    const double range = static_cast<double>(ramp_end - ramp_start + 1);
    if (m_state->turn_count >= ramp_start && m_state->turn_count <= ramp_end) {
        m_score_diff_weight += (0.75 - 0.25) / range;
        m_freq_count_diff_weight -= (0.40 - 0.15) / range;
        m_lock_progress_diff_weight -= (0.35 - 0.10) / range;

        //m_score_diff_scale_factor -= (30.0 - 1.0) / range;
        //m_freq_count_diff_scale_factor -= (static_cast<double>(m_max_frequency_count_left) - 1.0) / range;
        //m_lock_progress_diff_scale_factor -= (11.0 - 1.0) / range;
    }
    
    std::vector scores = compute_score();
    const int score_diff = scores[0] - scores[1];
    const double score_diff_term = m_score_diff_weight * std::max(-1.0, std::min(1.0, static_cast<double>(score_diff) / m_score_diff_scale_factor));

    std::array<int, 2> freq_count_left = {0, 0};
    for (size_t i = 0; i < 2; ++i) {
        for (size_t j = 0; j < GameConstants::NUM_ROWS; ++j) {
            if (m_state->locked_rows[j]) {
                continue;
            }

            const size_t start = m_state->scorepads[i].get_rightmost_mark_index(static_cast<Color>(j)).value_or(0);
            for (size_t k = start; k <= GameConstants::FIRST_LOCK_INDEX; ++k) {
                freq_count_left[i] += m_frequency_counts[k];
            }
        }
    }

    const int freq_count_diff = freq_count_left[0] - freq_count_left[1];
    const double freq_count_diff_term = m_freq_count_diff_weight * std::max(-1.0, std::min(1.0, (static_cast<double>(freq_count_diff) / m_freq_count_diff_scale_factor)));

    //const double num_locks_term = m_num_locks_weight * m_state->num_locks;

    auto get_lock_progress = [this](size_t player, Color color) {
        std::tuple<int, double> progress = {0, 0.0};

        if (m_state->locked_rows[static_cast<size_t>(color)]) {
            return progress;
        }

        const size_t num_marks = static_cast<size_t>(m_state->scorepads[player].get_num_marks(color));
        const size_t rightmost_index = m_state->scorepads[player].get_rightmost_mark_index(color).value_or(0);
        const size_t spaces_left = GameConstants::FIRST_LOCK_INDEX - rightmost_index + 1;
        const size_t marks_needed = GameConstants::MIN_MARKS_FOR_LOCK_ONE - num_marks;

        if (spaces_left < marks_needed) {
            progress = {0, -3.0};
            return progress;
        }

        if (num_marks >= 5) {
            progress = {5, 3.0};
            return progress;
        }
        
        int freq_count_left = 0;
        for (size_t i = rightmost_index + 1; i < GameConstants::FIRST_LOCK_INDEX; ++i) {
            freq_count_left += m_frequency_counts[i];
        }

        double freq_count_per_marks_needed = static_cast<double>(freq_count_left) / static_cast<double>(marks_needed);
        
        progress = {num_marks, std::max(-3.0, std::min(3.0, freq_count_per_marks_needed - 7.0))};
        return progress;
    };
    
    std::array<double, 2> lock_progress = {0.0, 0.0};
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

        lock_progress[i] = std::max(-1.0, std::min(1.0, (top_progress + bottom_progress - m_lock_progress_diff_bias) / m_lock_progress_diff_scale_factor));
    }

    const double lock_progress_diff = lock_progress[0] - lock_progress[1];
    const double lock_progress_diff_term = m_lock_progress_diff_weight * std::max(-1.0, std::min(1.0, lock_progress_diff));

    return score_diff_term + freq_count_diff_term + lock_progress_diff_term;

    /*std::cout << "Evaluation terms:\n"
              << "Score difference: " << score_diff_term << '\n'
              << "Frequency difference: " << freq_count_diff_term << '\n'
              << "Lock progress difference: " << lock_progress_diff_term << '\n'
              << "Number of locks: " << num_locks_term << '\n';*/

    //return (partial_sum > 0.0) ? (partial_sum + num_locks_term) : (partial_sum - num_locks_term); 
}

std::unique_ptr<GameData> Game::run() {        
    // Initial colors of the colored dice. Colored dice may be removed during the game.
    std::vector<Color> dice = { Color::red, Color::yellow, Color::green, Color::blue };

    // Value of dice rolls. The first two represent the white dice. The final four represent the colored dice.
    // Colored dice may be removed during the game.
    std::vector<int> rolls = { 0, 0, 0, 0, 0, 0 };

    std::array<Move, GameConstants::MAX_LEGAL_MOVES> current_action_legal_moves{};
    std::array<Move, GameConstants::MAX_LEGAL_MOVES> action_two_possible_moves{};
    std::array<std::optional<Move>, GameConstants::MAX_PLAYERS> registered_moves{};

    MoveContext ctxt = {
        std::span<Color>(dice),
        std::span<int>(rolls),
        std::span<Move>(current_action_legal_moves),
        std::span<Move>(action_two_possible_moves),
        std::span<std::optional<Move>>(registered_moves)
    };

    auto lock_added = [&]() {
        // Check each lock and remove the corresponding dice
        for (size_t i = 0; i < GameConstants::NUM_ROWS; ++i) {
            if (m_state->locks.test(i)) {
                m_state->locked_rows[i] = true;
                Color color_to_remove = static_cast<Color>(i);
                auto it = std::find(dice.begin(), dice.end(), color_to_remove);     // if the value is not found, it is a bug in the program
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
    
        // Check number of locks
        if (m_state->num_locks >= 2) {
            m_state->is_terminal = true;
        }
    };

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
        if (m_use_evaluation) {
            const double evaluation = evaluate_2p();
            if (m_human_active) {
                std::cout << "Evaluation for player 0: " << evaluation << '\n';
            }
            p0_evaluation_history.push_back(evaluation);
        }

        m_state->turn_count += 1;
        
        roll_dice(ctxt.rolls);

        if (m_human_active) {
            std::cout << "\nStarting new round.\nRolling dice...\n"
                    << "WHITE: " << ctxt.rolls[0] << ' ' << ctxt.rolls[1] << '\n';

            for (size_t i = 0; i < dice.size(); ++i) {
                std::cout << color_to_string[dice[i]] << ": " << ctxt.rolls[i + 2] << '\n';
            }

            std::cout << "Action one in progress. Player " << m_state->curr_player << " is active.\n";
        }

        active_player_made_move = resolve_action<ActionType::First>(ctxt, lock_added);

        if (m_state->is_terminal) {
            break;
        }

        if (m_human_active) {
            std::cout << "Action two in progress. Player " << m_state->curr_player << " is active.\n";
        }

        active_player_made_move |= resolve_action<ActionType::Second>(ctxt, lock_added);

        check_penalties(active_player_made_move);

        active_player_made_move = false;
        m_state->curr_player = (m_state->curr_player + 1) % m_num_players;

        /*char sink;
        std::cin >> sink;*/
    }

    std::vector<int> final_score = compute_score();

    int max_val = *std::max_element(final_score.begin(), final_score.end());
    std::vector<size_t> winners;
    for (size_t i = 0; i < final_score.size(); ++i) {
        if (final_score[i] == max_val) {
            winners.push_back(i);
        }
    }

    p0_evaluation_history.push_back(std::find(winners.begin(), winners.end(), 0) == winners.end() ? -1.0 : 1.0);

    std::unique_ptr<GameData> data = std::make_unique<GameData>(
        std::move(winners),
        std::move(final_score),
        std::move(m_state), 
        std::move(p0_evaluation_history),
        m_state->turn_count);

    return data;
}

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

template size_t generate_legal_moves<ActionType::First>(std::span<Move>& legal_moves, const std::span<Color>& dice, const std::span<int>& rolls, const Scorepad& scorepad);
template size_t generate_legal_moves<ActionType::Second>(std::span<Move>& legal_moves, const std::span<Color>& dice, const std::span<int>& rolls, const Scorepad& scorepad);
