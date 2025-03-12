#include <algorithm>
#include <array>
#include <cstdlib>
#include <ctime>
#include <iostream>
#include <memory>
#include <optional>
#include <span>

#include "game.hpp"

Scorepad::Scorepad() : m_rows{}, m_numMarks{}, m_penalties(0) {
    m_rightmostMarks.fill(GameConstants::NO_MARK);
    
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

void Scorepad::markMove(const Move& move) {
    size_t color = static_cast<size_t>(move.color);
    size_t index = move.index;
    m_rows[color][index].second = true;
    m_rightmostMarks[color] = getValueFromIndex(move.color, index);
    m_numMarks[color] += 1;
    if (index == (GameConstants::NUM_CELLS_PER_ROW - 1)) {
        m_numMarks[color] += 1;
    }
}

bool Scorepad::markPenalty() {
    return (++m_penalties >= 4);
}

State::State(size_t numPlayers, size_t startingPlayer) : scorepads{}, locks(false), currPlayer(startingPlayer), numLocks(0), isTerminal(false) {
    scorepads.reserve(numPlayers);
    for (size_t i = 0; i < numPlayers; ++i) {
        scorepads.push_back(Scorepad());
    }
}

std::optional<size_t> Agent::makeMove(std::span<const Move> moves, const State& state) const {
    const size_t moveIndex = rand() % (moves.size() + 1);
    return moveIndex == 0 ? std::nullopt : std::optional<size_t>(moveIndex - 1);
};

void rollDice(std::span<unsigned int> rolls) {
    for (size_t i = 0; i < rolls.size(); ++i) {
        rolls[i] = (rand() % 5) + 1;
    }
}

Game::Game(size_t numPlayers) : m_numPlayers(numPlayers) {
    if (numPlayers < GameConstants::MIN_PLAYERS || numPlayers > GameConstants::MAX_PLAYERS) {
        throw std::runtime_error("Invalid player count.");
    }

    for (size_t i = 0; i < numPlayers; ++i) {
        m_players.push_back(Agent());
    }
    
    m_state = std::make_unique<State>(numPlayers, rand() % numPlayers);
}

template <ActionType A>
size_t generateLegalMoves(const MoveContext& ctxt, const Scorepad& scorepad) {
    size_t numLegalMoves = 0;

    auto addMoveIfLegal = [&](const unsigned int valueToMark, const unsigned int rightmostMarkValue, const Color color, const size_t indexToMark) {
        // Is the number to mark after the rightmost-marked number on the row?
        if (valueToMark > rightmostMarkValue) {
            // Does the number have a lock? If so, have the minimum number of marks been placed to mark the lock?
            if (indexToMark < (GameConstants::NUM_CELLS_PER_ROW - 1) 
            || ((indexToMark == GameConstants::NUM_CELLS_PER_ROW) && (scorepad.getNumMarks(color) >= GameConstants::MIN_MARKS_FOR_LOCK)))
            {
                ctxt.legalMoves[numLegalMoves].color = color;
                ctxt.legalMoves[numLegalMoves].index = indexToMark;
                ++numLegalMoves;
            }
        }
    };
    
    unsigned int sum1 = 0;
    unsigned int sum2 = 0;

    // Use dice to get available color rows
    for (size_t i = 2; i < ctxt.rolls.size(); ++i) {
        if constexpr (A == ActionType::First) {
            sum1 = ctxt.rolls[0] + ctxt.rolls[1];
        }
        else {
            sum1 = ctxt.rolls[0] + ctxt.rolls[i];
            sum2 = ctxt.rolls[1] + ctxt.rolls[i];
        }

        const Color color = ctxt.dice[i - 2];
        const unsigned int rightmostMarkValue = scorepad.getRightmostMarkValue(color);
        const size_t indexToMark1 = scorepad.getIndexFromValue(color, sum1);
        const size_t indexToMark2 = scorepad.getIndexFromValue(color, sum2);

        if constexpr (A == ActionType::First) {
            addMoveIfLegal(sum1, rightmostMarkValue, color, indexToMark1);
        }
        else {
            addMoveIfLegal(sum1, rightmostMarkValue, color, indexToMark1);
            addMoveIfLegal(sum2, rightmostMarkValue, color, indexToMark2);
        }
    }

    return numLegalMoves;
}

template <ActionType A, typename F>
bool Game::resolveAction(const MoveContext& ctxt, F lockAdded) {
    bool activePlayerMadeMove = false;

    if constexpr (A == ActionType::First) {
        // Register first action moves
        for (size_t i = 0; i < m_numPlayers; ++i) {
            const unsigned int numMoves = generateLegalMoves<ActionType::First>(ctxt, m_state.get()->scorepads[i]);
            std::optional<size_t> moveIndex_opt = std::nullopt;
            if (numMoves > 0) {
                moveIndex_opt = m_players[i].makeMove(ctxt.legalMoves.subspan(0, numMoves), *m_state);
            }
            if (moveIndex_opt) {
                ctxt.registeredMoves[i] = ctxt.legalMoves[moveIndex_opt.value()];
                if (i == m_state->currPlayer) {
                    activePlayerMadeMove = true;
                }
            }
        }

        // Make first action moves
        for (size_t i = 0; i < m_numPlayers; ++i) {
            const std::optional<Move> move_opt = ctxt.registeredMoves[i];
            if (move_opt) {
                m_state.get()->scorepads[i].markMove(move_opt.value());
                if (move_opt.value().index == (GameConstants::NUM_CELLS_PER_ROW - 1)) {
                    m_state->locks[static_cast<size_t>(move_opt.value().color)] = true;
                }
            }
        }
    }
    else {
        const unsigned int numMoves = generateLegalMoves<ActionType::Second>(ctxt, m_state.get()->scorepads[m_state->currPlayer]);
        std::optional<size_t> moveIndex_opt = std::nullopt;
        if (numMoves > 0) {
            moveIndex_opt = m_players[m_state->currPlayer].makeMove(ctxt.legalMoves.subspan(0, numMoves), *m_state);
        }
        if (moveIndex_opt) {
            m_state.get()->scorepads[m_state->currPlayer].markMove(ctxt.legalMoves[moveIndex_opt.value()]);
            if (moveIndex_opt.value() == (GameConstants::NUM_CELLS_PER_ROW - 1)) {
                m_state->locks[static_cast<size_t>(ctxt.legalMoves[moveIndex_opt.value()].color)] = true;
            }
            activePlayerMadeMove = true;
        }
    }

    // Check locks and remove corresponding dice
    if (m_state->locks.any()) {
        lockAdded();
    }

    return activePlayerMadeMove;
}

void Game::computeScore(std::vector<int>& scores) {
    for (size_t i = 0; i < m_numPlayers; ++i) {
        int score = 0;
        for (size_t j = 0; j < GameConstants::NUM_ROWS; ++j) {
            int numMarks = m_state.get()->scorepads[i].getNumMarks(static_cast<Color>(j));
            score += (numMarks * (numMarks + 1)) / 2;
        }
        score -= m_state.get()->scorepads[i].getNumPenalties();
        scores.push_back(score);
    }
    std::reverse(scores.begin(), scores.end());
}

std::unique_ptr<GameData> Game::run() {        
    // Seed random number generator
    std::srand(std::time(nullptr));

    // Initial colors of the colored dice. Colored dice may be removed during the game.
    std::vector<Color> dice = { Color::red, Color::yellow, Color::green, Color::blue };

    // Value of dice rolls. The first two represent the white dice. The final four represent the colored dice.
    // Colored dice may be removed during the game.
    std::vector<unsigned int> rolls = { 0, 0, 0, 0, 0, 0 };

    std::array<Move, GameConstants::MAX_LEGAL_MOVES> legalMoves{};
    std::array<std::optional<Move>, GameConstants::MAX_PLAYERS> registeredMoves{};

    MoveContext ctxt = {
        std::span<Color>(dice),
        std::span<unsigned int>(rolls),
        std::span<Move>(legalMoves),
        std::span<std::optional<Move>>(registeredMoves)
    };

    auto lockAdded = [&]() {
        // Check each lock and remove the corresponding dice
        for (size_t i = 0; i < GameConstants::NUM_ROWS; ++i) {
            if (m_state->locks.test(i)) {
                dice.erase(dice.begin() + i);
                rolls.erase(rolls.begin() + 2 + i);
                ++(m_state->numLocks);
            }
        }

        // Reset the locks so that the next lock addition does not result in numLocks being incremented again for the current locks
        m_state->locks.reset();
    
        // Reconstruct spans for dice and rolls
        ctxt.dice = std::span<Color>(ctxt.dice);
        ctxt.rolls = std::span<unsigned int>(ctxt.rolls);
    
        // Check number of locks
        if (m_state->numLocks >= 2) {
            m_state->isTerminal = true;
        }
    };

    auto checkPenalties = [this](bool activePlayerMadeMove) {
        if (!activePlayerMadeMove) {
            if (m_state.get()->scorepads[m_state->currPlayer].markPenalty()) {
                m_state->isTerminal = true;
            }
        }
    };

    bool activePlayerMadeMove = false;
    
    while(!m_state->isTerminal) {
        rollDice(ctxt.rolls);
        activePlayerMadeMove = resolveAction<ActionType::First>(ctxt, lockAdded);
        activePlayerMadeMove &= resolveAction<ActionType::Second>(ctxt, lockAdded);
        checkPenalties(activePlayerMadeMove);
        activePlayerMadeMove = false;
        m_state->currPlayer = (m_state->currPlayer + 1) % m_numPlayers;
    }

    std::vector<int> finalScore{};
    finalScore.reserve(m_numPlayers);
    computeScore(finalScore);

    auto max_it = std::max_element(finalScore.begin(), finalScore.end());
    size_t max_index = static_cast<size_t>(std::distance(finalScore.begin(), max_it));
    std::vector<size_t> winners = { max_index };    // TODO: need to get all max value indices and add them to the list of winners

    std::unique_ptr<GameData> data = std::make_unique<GameData>(std::move(winners), std::move(finalScore), std::move(m_state));

    return data;
}