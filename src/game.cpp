#include <algorithm>
#include <array>
#include <cstdlib>
#include <ctime>
#include <iostream>
#include <memory>
#include <optional>
#include <span>

#include "game.hpp"

Scorepad::Scorepad() : m_rightmost_mark_indices{}, m_num_marks{}, m_penalties(0) {
    m_rightmost_mark_indices.fill(std::nullopt);
    
    for (size_t i = 0; i < m_rows.size(); ++i) {
        for (size_t j = 0; j < m_rows[i].size(); ++j) {
            if (static_cast<Color>(i) == Color::red || static_cast<Color>(i) == Color::yellow) {
                m_rows[i][j].first = j + 2;
                m_rows[i][j].second = false;
            }
            else {
                m_rows[i][j].first = 12 - j;
                m_rows[i][j].second = false;
            }
        }
    }
}

void Scorepad::mark_move(const Move& move) {
    size_t color = static_cast<size_t>(move.color);
    size_t index = move.index;
    m_rows[color][index].second = true;
    m_rightmost_mark_indices[color] = index;
    m_num_marks[color] += 1;
    if (index == (GameConstants::NUM_CELLS_PER_ROW - 1)) {
        m_num_marks[color] += 1;
    }
}

inline bool Scorepad::mark_penalty() {
    return (++m_penalties >= 4);
}

State::State(size_t numPlayers, size_t startingPlayer) 
    : scorepads(numPlayers, Scorepad()), locks(false), curr_player(startingPlayer), num_locks(0), is_terminal(false) {}

std::optional<size_t> Agent::make_move(std::span<const Move> moves) const {
    const size_t moveIndex = rand() % (moves.size() + 1);
    return moveIndex == 0 ? std::nullopt : std::optional<size_t>(moveIndex - 1);
};

void roll_dice(std::span<int> rolls) {
    for (size_t i = 0; i < rolls.size(); ++i) {
        rolls[i] = (rand() % 6) + 1;    // TODO: replace rand(), probably with mersenne twister
    }
}

template <ActionType A>
size_t generate_legal_moves(const MoveContext& ctxt, const Scorepad& scorepad) {
    size_t num_legal_moves = 0;

    auto add_move_if_legal = [&](const Color color, const std::optional<size_t> rightmost_mark_index, const size_t index_to_mark) {
        // Is the number to mark after the rightmost-marked number on the row?
        if (!rightmost_mark_index.has_value() || index_to_mark > rightmost_mark_index.value()) {
            // Does the number have a lock? If so, have the minimum number of marks been placed to mark the lock?
            if (index_to_mark < (GameConstants::NUM_CELLS_PER_ROW - 1) 
            || ((index_to_mark == (GameConstants::NUM_CELLS_PER_ROW - 1)) && (scorepad.get_num_marks(color) >= GameConstants::MIN_MARKS_FOR_LOCK)))
            {
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
        const size_t index_to_mark_1 = scorepad.get_index_from_value(color, sum_1);
        const size_t index_to_mark_2 = scorepad.get_index_from_value(color, sum_2);

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

Game::Game(size_t num_players) {
    if (num_players < GameConstants::MIN_PLAYERS || num_players > GameConstants::MAX_PLAYERS) {
        throw std::runtime_error("Invalid player count.");
    }

    m_num_players = num_players;

    for (size_t i = 0; i < num_players; ++i) {
        m_players.push_back(Agent());   // TODO: construct agents with different policies
    }
    
    m_state = std::make_unique<State>(num_players, rand() % num_players);     // TODO: replace rand()
}


template <ActionType A, typename F>
bool Game::resolve_action(const MoveContext& ctxt, F lock_added) {
    bool active_player_made_move = false;

    if constexpr (A == ActionType::First) {
        // Register first action moves
        for (size_t i = 0; i < m_num_players; ++i) {
            const int num_moves = generate_legal_moves<ActionType::First>(ctxt, m_state.get()->scorepads[i]);
            std::optional<size_t> move_index_opt = std::nullopt;
            if (num_moves > 0) {
                move_index_opt = m_players[i].make_move(ctxt.legal_moves.subspan(0, num_moves));
            }
            if (move_index_opt) {
                ctxt.registered_moves[i] = ctxt.legal_moves[move_index_opt.value()];
                if (i == m_state->curr_player) {
                    active_player_made_move = true;
                }
            }
        }

        // Make first action moves
        for (size_t i = 0; i < m_num_players; ++i) {
            const std::optional<Move> move_opt = ctxt.registered_moves[i];
            if (move_opt) {
                m_state.get()->scorepads[i].mark_move(move_opt.value());
                if (move_opt.value().index == (GameConstants::NUM_CELLS_PER_ROW - 1)) {
                    m_state->locks[static_cast<size_t>(move_opt.value().color)] = true;
                }
            }
        }
    }
    else {
        const int num_moves = generate_legal_moves<ActionType::Second>(ctxt, m_state.get()->scorepads[m_state->curr_player]);
        std::optional<size_t> move_index_opt = std::nullopt;
        if (num_moves > 0) {
            move_index_opt = m_players[m_state->curr_player].make_move(ctxt.legal_moves.subspan(0, num_moves));
        }
        if (move_index_opt) {
            m_state.get()->scorepads[m_state->curr_player].mark_move(ctxt.legal_moves[move_index_opt.value()]);
            if (move_index_opt.value() == (GameConstants::NUM_CELLS_PER_ROW - 1)) {
                m_state->locks[static_cast<size_t>(ctxt.legal_moves[move_index_opt.value()].color)] = true;
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

std::vector<int> Game::compute_score() const {
    std::vector<int> scores(m_num_players, 0);
    
    for (size_t i = 0; i < m_num_players; ++i) {
        int score = 0;
        for (size_t j = 0; j < GameConstants::NUM_ROWS; ++j) {
            int num_marks = m_state.get()->scorepads[i].get_num_marks(static_cast<Color>(j));
            score += (num_marks * (num_marks + 1)) / 2;
        }
        score -= m_state.get()->scorepads[i].get_num_penalties();
        scores[i] = score;
    }

    return scores;
}

std::unique_ptr<GameData> Game::run() {        
    // Seed random number generator
    std::srand(std::time(nullptr));     // TODO: replace

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
                dice.erase(dice.begin() + i);
                rolls.erase(rolls.begin() + 2 + i);
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
            m_state->is_terminal = true;
        }
    };

    auto checkPenalties = [this](bool activePlayerMadeMove) {
        if (!activePlayerMadeMove) {
            if (m_state.get()->scorepads[m_state->curr_player].mark_penalty()) {
                m_state->is_terminal = true;
            }
        }
    };

    bool activePlayerMadeMove = false;
    
    while(!m_state->is_terminal) {
        roll_dice(ctxt.rolls);

        activePlayerMadeMove = resolve_action<ActionType::First>(ctxt, lock_added);
        activePlayerMadeMove &= resolve_action<ActionType::Second>(ctxt, lock_added);
        checkPenalties(activePlayerMadeMove);

        activePlayerMadeMove = false;
        m_state->curr_player = (m_state->curr_player + 1) % m_num_players;
    }

    std::vector<int> final_score = compute_score();

    auto max_it = std::max_element(final_score.begin(), final_score.end());
    size_t max_index = static_cast<size_t>(std::distance(final_score.begin(), max_it));
    std::vector<size_t> winners = { max_index };    // TODO: need to get all max value indices and add them to the list of winners

    std::unique_ptr<GameData> data = std::make_unique<GameData>(std::move(winners), std::move(final_score), std::move(m_state));

    return data;
}