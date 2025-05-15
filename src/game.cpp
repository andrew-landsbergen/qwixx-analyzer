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
    //std::cout << "Marking " << color_to_string[move.color] << " " << index << '\n';
    m_rows[color][index] = true;
    //std::cout << "Rightmost mark index was " << (m_rightmost_mark_indices[color].has_value() ? m_rightmost_mark_indices[color].value() : -1) << '\n';
    m_rightmost_mark_indices[color] = index;
    //std::cout << "Rightmost mark index is now " << m_rightmost_mark_indices[color].value() << '\n';
    m_num_marks[color] += 1;
    if (index == (GameConstants::LOCK_INDEX)) {
        //std::cout << "Adding second mark for " << color_to_string[move.color] << index << "(lock)\n";
        m_num_marks[color] += 1;
    }
}

void roll_dice(std::span<int> rolls) {
    static std::uniform_int_distribution<int> dist(1, 6);
    for (size_t i = 0; i < rolls.size(); ++i) {
        rolls[i] = dist(rng());
    }
}

template <ActionType A>
size_t generate_legal_moves(const MoveContext& ctxt, const Scorepad& scorepad) {
    size_t num_legal_moves = 0;

    auto add_move_if_legal = [&](const Color color, const std::optional<size_t> rightmost_mark_index, const size_t index_to_mark) {
        // Is the number to mark after the rightmost-marked number on the row?
        /*std::cout << "Checking if move is legal:\n" << "Color: " << color_to_string[color] << '\n'
                  << "Index to mark: " << index_to_mark << '\n' 
                  << "Rightmost mark index: " << (!rightmost_mark_index.has_value() ? "No mark" : std::to_string(rightmost_mark_index.value())) << '\n';*/
        if (!rightmost_mark_index.has_value() || index_to_mark > rightmost_mark_index.value()) { 
            // Are we marking a lock? If so, have the minimum number of marks been placed to mark the lock?
            if (index_to_mark < (GameConstants::LOCK_INDEX) 
            || ((index_to_mark == GameConstants::LOCK_INDEX) && (scorepad.get_num_marks(color) >= GameConstants::MIN_MARKS_FOR_LOCK)))
            {
                //std::cout << "This move is legal.\n";
                ctxt.legal_moves[num_legal_moves].color = color;
                ctxt.legal_moves[num_legal_moves].index = index_to_mark;
                ++num_legal_moves;
            }
        }
    };
    
    int sum_1 = 0;
    int sum_2 = 0;

    // Use dice to get available color rows
    for (size_t i = 2; i < ctxt.rolls.size(); ++i) {
        if constexpr (A == ActionType::First) {
            sum_1 = ctxt.rolls[0] + ctxt.rolls[1];
        }
        else {
            sum_1 = ctxt.rolls[0] + ctxt.rolls[i];
            sum_2 = ctxt.rolls[1] + ctxt.rolls[i];
        }

        const Color color = ctxt.dice[i - 2];
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

Game::Game(std::vector<Agent*> players) : m_num_players(players.size()), m_players(players) {    
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
        //std::cout << "Computing score for player " << i << '\n';
        int score = 0;
        for (size_t j = 0; j < GameConstants::NUM_ROWS; ++j) {
            int num_marks = m_state.get()->scorepads[i].get_num_marks(static_cast<Color>(j));
            //std::cout << "Number of marks in row " << j << ": " << num_marks << '\n';
            score += (num_marks * (num_marks + 1)) / 2;
            //std::cout << "Score after adding row " << j << ": " << score << '\n';
        }
        //std::cout << "Sum of row scores: " << score << '\n';
        score -= GameConstants::PENALTY_VALUE * (m_state.get()->scorepads[i].get_num_penalties());
        //std::cout << "Score after deducting penalty points: " << score << '\n';
        scores[i] = score;
    }

    return scores;
}

std::unique_ptr<GameData> Game::run() {        
    //std::cout << "Game started.\n";

    // Initial colors of the colored dice. Colored dice may be removed during the game.
    std::vector<Color> dice = { Color::red, Color::yellow, Color::green, Color::blue };

    // Value of dice rolls. The first two represent the white dice. The final four represent the colored dice.
    // Colored dice may be removed during the game.
    std::vector<int> rolls = { 0, 0, 0, 0, 0, 0 };

    std::array<Move, GameConstants::MAX_LEGAL_MOVES> legal_moves{};
    std::array<std::optional<Move>, GameConstants::MAX_PLAYERS> registered_moves{};

    MoveContext ctxt = {
        std::span<Color>(dice),
        std::span<int>(rolls),
        std::span<Move>(legal_moves),
        std::span<std::optional<Move>>(registered_moves)
    };

    auto lock_added = [&]() {
        // Check each lock and remove the corresponding dice
        for (size_t i = 0; i < GameConstants::NUM_ROWS; ++i) {
            if (m_state->locks.test(i)) {
                Color color_to_remove = static_cast<Color>(i);
                auto it = std::find(dice.begin(), dice.end(), color_to_remove);     // if the value is not found, it is a bug in the program
                dice.erase(it);
                int dist = std::distance(dice.begin(), it);
                rolls.erase(rolls.begin() + (dist + 2));
                ++(m_state->num_locks);
            }
        }

        // Reset the locks so that the next lock addition does not result in numLocks being incremented again for the current locks
        m_state->locks.reset();
    
        // Reconstruct spans for dice and rolls
        ctxt.dice = std::span<Color>(ctxt.dice);
        ctxt.rolls = std::span<int>(ctxt.rolls);
    
        // Check number of locks
        if (m_state->num_locks >= 2) {
            //std::cout << "The total number of locks is at least 2. The game is now over.\n";
            m_state->is_terminal = true;
        }
    };

    auto check_penalties = [this](bool active_player_made_move) {
        if (!active_player_made_move) {
            //std::cout << "The active player did not make a move during either action. They must mark a penalty box.\n";
            if (m_state.get()->scorepads[m_state->curr_player].mark_penalty()) {
                //std::cout << "Four penalties have been marked. The game is now over.\n";
                m_state->is_terminal = true;
            }
        }
    };

    bool active_player_made_move = false;
    
    while(!m_state->is_terminal) {
        /*std::cout << "The active player is " << m_state->curr_player << ".\n";

        std::cout << "Rolling dice.\n";*/
        
        roll_dice(ctxt.rolls);

        /*std::cout << "The dice rolls are:\n";
    
        for (size_t i = 0; i < 2; ++i) {
            std::cout << "WHITE: " << ctxt.rolls[i] << '\n';
        }
    
        for (size_t i = 2; i < ctxt.rolls.size(); ++i) {
            Color color = ctxt.dice[i - 2];
            std::cout << color_to_string[color] << ": " << ctxt.rolls[i] << '\n';
        }*/

        active_player_made_move = resolve_action<ActionType::First>(ctxt, lock_added);

        if (m_state->is_terminal) {
            break;
        }

        active_player_made_move |= resolve_action<ActionType::Second>(ctxt, lock_added);

        check_penalties(active_player_made_move);

        active_player_made_move = false;
        m_state->curr_player = (m_state->curr_player + 1) % m_num_players;

        /*char sink;
        std::cin >> sink;*/
    }

    std::vector<int> final_score = compute_score();

    auto max_it = std::max_element(final_score.begin(), final_score.end());
    size_t max_index = static_cast<size_t>(std::distance(final_score.begin(), max_it));
    std::vector<size_t> winners = { max_index };    // TODO: this does not report ties

    std::unique_ptr<GameData> data = std::make_unique<GameData>(std::move(winners), std::move(final_score), std::move(m_state));

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

    for (auto move : ctxt.legal_moves) {
        Color color = move.color;
        int value = index_to_value(color, move.index);
        os << "{ " << color_to_string[color] << ' ' << value << " }, ";
    }

    os << '\n';

    return os;
}

template size_t generate_legal_moves<ActionType::First>(const MoveContext& ctxt, const Scorepad& scorepad);
template size_t generate_legal_moves<ActionType::Second>(const MoveContext& ctxt, const Scorepad& scorepad);
