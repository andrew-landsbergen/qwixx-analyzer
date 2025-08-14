#pragma once

#include <memory>
#include <optional>
#include <span>

#include "globals.hpp"

struct Move;
class State;

/**
 * @class Agent agent.hpp "src/agent.hpp"
 * @brief Methods and data for agents capable of playing Qwixx.
 * @details This is the base class from which other agent types can be derived. It consists of a make_move() method
 * which takes in a variety of data related to the current game state and returns the move chosen by the agent. Its
 * position (seating) in the game is set to 0 on construction, and must be explicitly set to the desired value with
 * the set_position() method. The reason for this quirk is that agents are initially constructed in main.cpp, which
 * is not responsible for running the game. When Game::run() is called, the position of each agent is set accordingly.
 */
class Agent {
public:
    /// @brief Default constructor, initializes m_position to 0.
    Agent() : m_position(0) {};

    /**
     * @brief Default destructor.
     * @details Note that this must be included to prevent a subtle compilation error.
     */
    virtual ~Agent() = default;
    
    /**
     * @brief Function used by each agent to choose its move.
     * @details How each agent chooses its move is determined by its policy, which is equivalent to the function
     * that overrides this virtual function. It returns a number corresponding to an index into current_action_legal_moves
     * or the null option when passing.
     * @param first_action A bool that is set to true if the current action to make a move for is the first action, else false.
     * Included to help agents plan their moves.
     * @param current_action_legal_moves A span of read-only Move objects indicating which moves are currently possible.
     * @param action_two_possible_moves A span of read-only Move objects indicating which moves are possible as part of the second action.
     * Included to help agents plan their moves. This span will hold the same values as current_action_legal_moves if first_action is false.
     * @param state A read-only reference to the current game state.
     * @return A size_t option which is expected to equal an index into current_action_legal_moves or the null option. The null option
     * represents passing.
     */
    virtual std::optional<size_t> make_move(bool first_action, std::span<const Move> current_action_legal_moves, std::span<const Move> action_two_possible_moves, const State& state) = 0;

    /**
     * @brief Sets the agent's position in the game.
     * @param position A size_t representing the new position of the agent.
     */
    void set_position(size_t position) {
        m_position = position;
    }
protected:
    size_t m_position;  //< The agent's position in the game.
};

/**
 * @class Human agent.hpp "src/agent.hpp"
 * @brief Methods and data for the human agent.
 * @details See the definition of Human::make_move() in src/agent.cpp.
 */
class Human : public Agent {
public:
    /// @brief Default constructor, uses the base class constructor.
    Human() : Agent() {};

    /**
     * @brief Function used by the human agent to determine their move.
     * @details See the documentation for make_move() in the Agent base class.
     */
    std::optional<size_t> make_move(bool first_action, std::span<const Move> current_action_legal_moves, std::span<const Move> action_two_possible_moves, const State& state) override;
};
    
/**
 * @class Random agent.hpp "src/agent.hpp"
 * @brief Methods and data for the random agent.
 * @details See the definition of Random::make_move() in src/agent.cpp.
 */
class Random : public Agent {
public:
    /// @brief Default constructor, uses the base class constructor.
    Random() : Agent() {};

    /**
     * @brief Function used by the random agent to determine its move.
     * @details See the documentation for make_move() in the Agent base class.
     */
    std::optional<size_t> make_move(bool first_action, std::span<const Move> current_action_legal_moves, std::span<const Move> action_two_possible_moves, const State& state) override;
};

/**
 * @class Greedy agent.hpp "src/agent.hpp"
 * @brief Methods and data for the greedy agent.
 * @details See the definition of Greedy::make_move() in src/agent.cpp.
 */
class Greedy : public Agent {
public:
    /**
     * @brief Default constructor, uses the base class constructor and then initializes m_max_skips to the value passed in.
     * @details The greedy agent has a maximum number of spaces on the scorepad that it can skip, which is set during construction.
     * @param max_skips An int representing the maximum number of spaces this agent can skip with its moves.
     */
    Greedy(int max_skips) : Agent(), m_max_skips(max_skips) {};

    /**
     * @brief Function used by the greedy agent to determine its move.
     * @details See the documentation for make_move() in the Agent base class.
     */
    std::optional<size_t> make_move(bool first_action, std::span<const Move> current_action_legal_moves, std::span<const Move> action_two_possible_moves, const State& state) override;
protected:
    int m_max_skips;    //< The maximum number of spaces this agent can skip with its moves.
};

/**
 * @class GreedyImproved agent.hpp "src/agent.hpp"
 * @brief Methods and data for the improved greedy agent.
 * @details See the definition of GreedyImproved::make_move() in src/agent.cpp.
 */
class GreedyImproved : public Agent {
public:
    /**
     * @brief Default constructor, uses the base class constructor and then initializes m_standard_max_skips to the value passed in.
     * @details The improved greedy agent has a standard maximum number of spaces on the scorepad that it can skip, which is set 
     * during construction. This standard maximum can increase by 1 when trying to avoid a penalty.
     * @param max_skips An int representing the maximum number of spaces this agent can skip with its moves.
     */
    GreedyImproved(int max_skips) : Agent(), m_standard_max_skips(max_skips) {};

    /**
     * @brief Function used by the improved greedy agent to determine its move.
     * @details See the documentation for make_move() in the Agent base class.
     */
    std::optional<size_t> make_move(bool first_action, std::span<const Move> current_action_legal_moves, std::span<const Move> action_two_possible_moves, const State& state) override;
protected:
    bool m_made_first_action_move = false;      //< Used by the agent to check if it made a move during the first action.
    int m_standard_max_skips;       //< The standard maximum number of spaces this agent can skip with its moves.
    int m_standard_max_penalty_avoidance_skips = 1;     // The maximum increase to the standard maximum this agent can skip when avoiding a penalty.
};

/**
 * @class RushLocks agent.hpp "src/agent.hpp"
 * @brief Methods and data for the rush agent.
 * @details See the definition of RushLocks::make_move() in src/agent.cpp.
 */
class RushLocks : public Agent {
public:
    /// @brief Default constructor, uses the base class constructor.
    RushLocks() : Agent() {};

    /**
     * @brief Function used by the rush agent to determine its move.
     * @details See the documentation for make_move() in the Agent base class.
     */
    std::optional<size_t> make_move(bool first_action, std::span<const Move> current_action_legal_moves, std::span<const Move> action_two_possible_moves, const State& state) override;
protected:
    bool m_made_first_action_move = false;      //< Used by the agent to check if it made a move during the first action.
    Color m_top_row_fast = Color::red;          //< Fast row between red and yellow.
    Color m_bottom_row_fast = Color::green;     //< Fast row between green and blue.
};

/**
 * @class Computational agent.hpp "src/agent.hpp"
 * @brief Methods and data for the computational agent.
 * @details See the definition of Computational::make_move() in src/agent.cpp.
 */
class Computational : public Agent {
public:
    Computational();

    /**
     * @struct MoveData
     * @brief Contains values representing the base penalty and roll frequency of a move.
     */
    struct MoveData {
        int base_penalty;
        int roll_frequency;
    };

    /**
     * @brief Function used by the computational agent to determine its move.
     * @details See the documentation for make_move() in the Agent base class.
     */
    std::optional<size_t> make_move(bool first_action, std::span<const Move> current_action_legal_moves, std::span<const Move> action_two_possible_moves, const State& state) override;
protected:
    bool m_made_first_action_move = false;      //< Used by the agent to check if it made a move during the first action.
    double m_alpha = 0.949905;      //< The alpha parameter is a discount factor for losing access to the move in the future.
    double m_mu = 0.49005;          //< The mu parameter is a discount factor for not likely being able to mark all spaces to the right of the move.
    double m_delta = 0.823284;      //< The delta parameter is a discount factor for losing access to moves to the left of the current move in the future.
    double m_sigma = 0.921692;      //< The sigma parameter is a discount factor for losing access to moves to the left of the current move in the future.
    double m_epsilon = 0.71407;     //< The epsilon parameter is an estiamte of the total fraction of all spaces on the scorepad that will be filled by the game's end.
    std::array<MoveData, GameConstants::NUM_CELLS_PER_ROW> m_basic_values;  //< Holds the basic values (base penalty and roll frequency) for each move.
};