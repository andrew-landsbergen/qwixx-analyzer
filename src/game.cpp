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

#include "game.hpp"

std::mt19937_64 rng;

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
    if (index == (GameConstants::LOCK_INDEX)) {
        m_num_marks[color] += 1;
    }
}

inline bool Scorepad::mark_penalty() {
    return (++m_penalties >= 4);
}

State::State(size_t numPlayers, size_t startingPlayer) 
    : scorepads(numPlayers, Scorepad()), locks(false), curr_player(startingPlayer), num_locks(0), is_terminal(false) {}

std::optional<size_t> Agent::make_move(std::span<const Move> moves) const {
    /*std::cout << "The legal moves are:\n";

    for (auto move : moves) {
        Color color = move.color;
        int value = index_to_value(color, move.index);
        std::cout << "{ " << color_to_string[color] << ' ' << value << " }, ";
    }

    std::cout << '\n';*/
    
    std::uniform_int_distribution<size_t> dist(0, moves.size());
    const size_t move_index = dist(rng);
    return move_index == 0 ? std::nullopt : std::optional<size_t>(move_index - 1);
};

void roll_dice(std::span<int> rolls) {
    static std::uniform_int_distribution<int> dist(1, 6);
    for (size_t i = 0; i < rolls.size(); ++i) {
        rolls[i] = dist(rng);
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
            // Does the number have a lock? If so, have the minimum number of marks been placed to mark the lock?
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

Game::Game(size_t num_players) {
    if (num_players < GameConstants::MIN_PLAYERS || num_players > GameConstants::MAX_PLAYERS) {
        throw std::runtime_error("Invalid player count.");
    }

    m_num_players = num_players;

    for (size_t i = 0; i < num_players; ++i) {
        m_players.push_back(Agent());   // TODO: construct agents with different policies
    }
    
    std::uniform_int_distribution<size_t> dist(0, num_players - 1);
    m_state = std::make_unique<State>(num_players, dist(rng));
}


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
                move_index_opt = m_players[i].make_move(ctxt.legal_moves.subspan(0, num_moves));
            }

            if (move_index_opt) {
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
                //std::cout << "Player " << i << " has opted to pass.\n";
            }
        }

        // Make first action moves
        for (size_t i = 0; i < m_num_players; ++i) {
            const std::optional<Move> move_opt = ctxt.registered_moves[i];
            if (move_opt) {
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
    else {
        /*std::cout << "Resolving second action.\n";

        std::cout << "Player " << m_state->curr_player << "'s scorepad:\n" << m_state.get()->scorepads[m_state->curr_player];*/
        
        const int num_moves = generate_legal_moves<ActionType::Second>(ctxt, m_state.get()->scorepads[m_state->curr_player]);

        std::optional<size_t> move_index_opt = std::nullopt;
        if (num_moves > 0) {
            move_index_opt = m_players[m_state->curr_player].make_move(ctxt.legal_moves.subspan(0, num_moves));
        }
        if (move_index_opt) {
            /*std::cout << "Active player" << " has selected move " << "{ "
            << color_to_string[ctxt.legal_moves[move_index_opt.value()].color]
            << ' '
            << index_to_value(ctxt.legal_moves[move_index_opt.value()].color, ctxt.legal_moves[move_index_opt.value()].index)
            << " }\n";*/

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
    //std::cout << "Game started.\n";
    
    // Seed random number generator
    rng.seed(std::chrono::system_clock::now().time_since_epoch().count());

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

    auto checkPenalties = [this](bool activePlayerMadeMove) {
        if (!activePlayerMadeMove) {
            //std::cout << "The active player did not make a move during either action. They must mark a penalty box.\n";
            if (m_state.get()->scorepads[m_state->curr_player].mark_penalty()) {
                //std::cout << "Four penalties have been marked. The game is now over.\n";
                m_state->is_terminal = true;
            }
        }
    };

    bool activePlayerMadeMove = false;
    
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

        activePlayerMadeMove = resolve_action<ActionType::First>(ctxt, lock_added);

        if (m_state->is_terminal) {
            break;
        }

        activePlayerMadeMove |= resolve_action<ActionType::Second>(ctxt, lock_added);

        checkPenalties(activePlayerMadeMove);

        activePlayerMadeMove = false;
        m_state->curr_player = (m_state->curr_player + 1) % m_num_players;

        /*char sink;
        std::cin >> sink;*/
    }

    std::vector<int> final_score = compute_score();

    auto max_it = std::max_element(final_score.begin(), final_score.end());
    size_t max_index = static_cast<size_t>(std::distance(final_score.begin(), max_it));
    std::vector<size_t> winners = { max_index };    // TODO: need to get all max value indices and add them to the list of winners

    std::unique_ptr<GameData> data = std::make_unique<GameData>(std::move(winners), std::move(final_score), std::move(m_state));

    return data;
}

constexpr int index_to_value(Color color, size_t index) {
    if (color == Color::red || color == Color::yellow) {
        return static_cast<int>(index + 2);
    }
    else {
        return static_cast<int>(12 - index);
    }
}

constexpr size_t value_to_index(Color color, int value) {
    if (color == Color::red || color == Color::yellow) {
        return static_cast<size_t>(value) - 2;
    }
    else {
        return 12 - static_cast<size_t>(value);
    }
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
        for (auto pair : scorepad.m_rows[i]) {
            stream << std::setw(4) << (pair.second ? "X" : std::to_string(pair.first));
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