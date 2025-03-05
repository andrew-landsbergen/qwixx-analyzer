#include <algorithm>
#include <array>
#include <cstdlib>
#include <ctime>
#include <optional>

#include "game.hpp"

void Scorepad::markNumberAtRow(const Move& move) {
    int color = static_cast<int>(move.row);
    int index = move.index;
    m_rows[color][index] = true;

    if (index > m_rightmostMarks[color]) {
        m_rightmostMarks[color] = index;
    }
}

bool Scorepad::markPenalty() {
    return (++m_penalties >= 4);
}

int Scorepad::getRightmostMarkIndex(Color color) const {
    return m_rightmostMarks[static_cast<int>(color)];
}

int Agent::makeMove(Move **legalMoves, size_t numMoves, State *state) {
    return rand() % numMoves;
};

Game::Game(int n) {
    if (n < 2 || n > 5) {
        throw std::runtime_error("Player count must be between 2 and 5.");
    }

    std::vector<std::unique_ptr<Scorepad>> scorepads;
    scorepads.reserve(n);

    for (int i = 0; i < n; ++i) {
        scorepads.push_back(std::make_unique<Scorepad>());
        players.push_back(std::make_unique<Agent>());
    }
    
    state = std::make_unique<State>(scorepads, rand() % n);

    std::cout << "Game started." << '\n';
}

Move **generateLegalMoves(bool isFirstAction, Scorepad *scorepad, int dice[], unsigned int rolls[], unsigned int numDice) {
    // TODO: add lock check
    
    Move **moves;
    // At most 4 valid moves on the first action and 8 on the second action, so take the max
    moves = new Move*[8];
    moves[0] = nullptr;     // used to determine if there are no legal moves
    unsigned int nextSlot = 0;
    if (isFirstAction) {
        // TODO: bounds check
        unsigned int sum = rolls[0] + rolls[1];

        // Use dice to get available color rows
        for (unsigned int i = 2; i < numDice; ++i) {
            int color = dice[i];
            int rightmostMarkIndex = scorepad->getRightmostMarkIndex(static_cast<Color>(color));
            if (color == static_cast<int>(Color::red) || color == static_cast<int>(Color::yellow)) {
                int rightmostMarkValue = rightmostMarkIndex + 2;
                if (sum > rightmostMarkValue) {
                    Move *move = new Move;
                    move->row = color;
                    move->index = sum - 2;
                    moves[nextSlot++] = move;
                }
            }
            else {
                int rightmostMarkValue = 12 - rightmostMarkIndex;
                if (sum < rightmostMarkValue) {
                    Move *move = new Move;
                    move->row = color;
                    move->index = 12 - sum;
                    moves[nextSlot++] = move;
                }
            }
        }
    }
    else {
        for (unsigned int i = 2; i < numDice; ++i) {
            unsigned int sum1 = rolls[0] + rolls[i];
            unsigned int sum2 = rolls[1] + rolls[i];

            int color = dice[i];
            int rightmostMarkIndex = scorepad->getRightmostMarkIndex(static_cast<Color>(color));
            
            if (color == static_cast<int>(Color::red) || color == static_cast<int>(Color::yellow)) {
                int rightmostMarkValue = rightmostMarkIndex + 2;
                if (sum1 > rightmostMarkValue) {
                    Move *move = new Move;
                    move->row = color;
                    move->index = sum1 - 2;
                    moves[nextSlot++] = move;
                }

                if (sum2 > rightmostMarkValue) {
                    Move *move = new Move;
                    move->row = color;
                    move->index = sum2 - 2;
                    moves[nextSlot++] = move;
                }
            }
            else {
                int rightmostMarkValue = 12 - rightmostMarkIndex;
                if (sum1 < rightmostMarkValue) {
                    Move *move = new Move;
                    move->row = color;
                    move->index = 12 - sum1;
                    moves[nextSlot++] = move;
                }

                if (sum2 < rightmostMarkValue) {
                    Move *move = new Move;
                    move->row = color;
                    move->index = 12 - sum2;
                    moves[nextSlot++] = move;
                }
            }
        }
    }
    return moves;
}

State *Game::run() {
    std::srand(std::time(nullptr));
    //int dice[6] = { -1, -1, 0, 1, 2, 3 };   // fix
    unsigned int numDice = 6;
    unsigned int rolls[6];

    while(!state->terminal) {
        // Roll dice
        for (unsigned int i = 0; i < numDice; ++i) {
            rolls[i] = (rand() % 5) + 1;
        }

        /*// First action
        for (int i = 0; i < state->numPlayers; ++i) {
            Move **moves = generateLegalMoves(true, state->scorepads[i], dice, rolls, numDice);
            players[i]->makeMove(moves, 8, state);
        }*/
    }

    return state.get();
}