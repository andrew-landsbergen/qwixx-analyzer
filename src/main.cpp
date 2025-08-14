#include <chrono>
#include <cmath>
#include <iostream>
#include <sstream>
#include <tuple>

#include "agent.hpp"
#include "game.hpp"
#include "rng.hpp"

double compute_duration(std::vector<std::vector<double>>& evaluation_histories);
double compute_lead_change(std::vector<std::vector<double>>& evaluation_histories);
double compute_late_uncertainty(std::vector<std::vector<double>>& evaluation_histories);
std::vector<int> get_inputs();
std::vector<std::tuple<std::unique_ptr<Agent>, std::string>> get_players(const std::vector<int>& inputs);
bool is_human_active(const std::vector<int>& inputs, int human_id);

/**
 * @brief Program entry point.
 * @details The main function gets input from the user about the number of simulations to run, whether the
 * evaluation function should be used, and which agents to use. Each simulation is then run and data are
 * collected for the complete trial, including the minimum, maximum, and average number of moves, the
 * average duration, lead change, and uncertainty (late), and a random evaluation history from the trial.
 * These data are printed to stdout, and then the program terminates.
 * @return An integer representing the exit status.
 */
int main() {
    const std::vector<int> inputs = get_inputs();

    const int num_simulations = inputs[0];
    const int use_evaluation = inputs[1];
    
    const std::vector<std::tuple<std::unique_ptr<Agent>, std::string>> players = get_players(inputs);
    const bool human_active = is_human_active(inputs, 23);

    // Start timer after collecting inputs
    auto start = std::chrono::high_resolution_clock::now();

    // Extract the pointers to the agents from the players vector, as these are
    // expected by the Game class
    std::vector<Agent*> player_ptrs;
    for (const auto& player : players) {
        player_ptrs.push_back(std::get<0>(player).get());
    }

    // Containers for statistics
    std::vector<double> num_wins_accum(players.size());
    std::vector<int> score_accum(players.size());
    int num_turns_accum = 0;
    int min_turns = std::numeric_limits<int>::max();
    int max_turns = std::numeric_limits<int>::min();

    // Container for evaluation histories, used by compute_duration(), compute_lead_change(), and compute_late_uncertainty().
    // It would be ideal to rewrite those functions so that the relevant statistic can be computed for each simulation and then
    // taking the average at the end, so that we don't have to hold all these vectors in memory.
    std::vector<std::vector<double>> evaluation_histories(num_simulations);

    // Determine the randomly-chosen simulation number whose evaluation history we will output at the end
    std::uniform_int_distribution<int> dist(0, num_simulations - 1);
    int random_sim = dist(rng());
    std::vector<double> saved_history;

    //int p1_mutation_score = 0;
    //int p2_mutation_score = 0;
    //int games_per_mutation = 500;
    
    //std::array<double, 24> p0_evaluation_by_turn{};
    //int num_games_tracked = 0;

    // Main program loop
    for (int i = 0; i < num_simulations; ++i) {
        /*if (i % games_per_mutation == 0) {
            bool final_mutation = (i + games_per_mutation >= num_simulations) ? true : false;
            player_ptrs[0]->mutate(static_cast<double>(p1_mutation_score) / static_cast<double>(games_per_mutation), final_mutation);
            player_ptrs[1]->mutate(static_cast<double>(p2_mutation_score) / static_cast<double>(games_per_mutation), final_mutation);
            p1_mutation_score = 0;
            p2_mutation_score = 0;
        }*/

        // Construct and run a new game
        Game game = Game(player_ptrs, human_active, (static_cast<bool>(use_evaluation) && players.size() == 2));
        std::unique_ptr<GameData> stats = game.run();

        // If we are at the randomly-chosen simulation number, save this game history
        if (i == random_sim && static_cast<bool>(use_evaluation)) {
            saved_history = stats.get()->p0_evaluation_history;
        }

        // Add wins and scores for each player to the relevant accumulators
        for (size_t j = 0; j < players.size(); ++j) {
            if (std::find(stats.get()->winners.begin(), stats.get()->winners.end(), j) != stats.get()->winners.end()) {
                num_wins_accum[j] += 1.0 / static_cast<double>(stats.get()->winners.size());
            }

            score_accum[j] += stats.get()->final_score[j];
        }

        // Move this game's evaluation history into the vector of all evaluation histories
        evaluation_histories[i] = std::move(stats.get()->p0_evaluation_history);

        // Update the accumulator for the number of turns, as well as the minimum and maximum numbers of turns, if applicable
        num_turns_accum += stats.get()->num_turns;
        min_turns = std::min(min_turns, stats.get()->num_turns);
        max_turns = std::max(max_turns, stats.get()->num_turns);

        /*if (stats.get()->winners[0] == 0 && stats.get()->p0_evaluation_history.size() == 24) {
            for (size_t i = 0; i < 23; ++i) {
                p0_evaluation_by_turn[i] += stats.get()->p0_evaluation_history[i];
            }
            num_games_tracked += 1;
        }*/

        //p1_mutation_score += stats.get()->final_score[0];
        //p2_mutation_score += stats.get()->final_score[1];
    }

    // Print win rates and average scores for each player
    for (size_t i = 0; i < players.size(); ++i) {
        std::cout << "Player " << i << " (" << std::get<1>(players[i]) << ") win rate: " << num_wins_accum[i] / static_cast<double>(num_simulations) << '\n';
        std::cout << "Player " << i << " (" << std::get<1>(players[i]) << ") average score: " << static_cast<double>(score_accum[i]) / static_cast<double>(num_simulations) << '\n';
    }

    // Print average, max, and min number of turns
    std::cout << "Average number of turns: " << static_cast<double>(num_turns_accum) / static_cast<double>(num_simulations) << '\n';
    std::cout << "Maximum number of turns: " << max_turns << '\n';
    std::cout << "Minimum number of turns: " << min_turns << '\n';

    if (static_cast<bool>(use_evaluation)) {
        // Compute duration, lead change, and uncertainty (late) statistics
        const double duration_stat = compute_duration(evaluation_histories);
        const double lead_change_stat = compute_lead_change(evaluation_histories);
        const double late_uncertainty_stat = compute_late_uncertainty(evaluation_histories);

        // Print the statistics
        std::cout << "Duration statistic: " << duration_stat << '\n';
        std::cout << "Lead change statistic: " << lead_change_stat << '\n';
        std::cout << "Uncertainty (late) statistic: " << late_uncertainty_stat << '\n';

        // Print the randomly-chosen evaluation history
        std::cout << "Randomly selected evaluation history (simulation #" << random_sim << "):\n";
        for (size_t i = 0; i < saved_history.size(); ++i) {
            std::cout << i << ", " << saved_history[i] << '\n';
        }
    }

    /*if (num_games_tracked > 0) {
        for (size_t i = 0; i < 23; ++i) {
            p0_evaluation_by_turn[i] /= static_cast<double>(num_games_tracked);
        }
    }*/

    /*std::cout << "Player 0 evaluation by turn (" << num_games_tracked << " won games with exactly 23 turns):\n";
    for (size_t i = 0; i < 24; ++i) {
        std::cout << i << ", " << p0_evaluation_by_turn[i] << '\n';
    }*/

    // Stop timer and print execution time
    auto end = std::chrono::high_resolution_clock::now();
    std::chrono::duration<double> duration = end - start;
    std::cout << "Completed in " << duration.count() << " seconds\n";

    return 0;
}

/**
 * @brief Function to compute the duration quality criterion.
 * @details Calculates the average duration, measured as the deviation in the number of moves (M_g)
 * from the preferred number of moves (M_pref). M_pref is assumed to be equal to the average number
 * of moves over all games in the trial for simplicity.
 * @param evaluation_histories A vector of vectors of doubles containing evaluations with respect to player 0 for all games.
 * @return A double in the interval [0, 1] representing the average duration. 0 indicates no deviation from
 * the preferred number of turns, while 1 indicates maximum deviation from the preferred number of turns. The maximum is
 * reached for games with 0 or 2M_g moves.
 */
double compute_duration(std::vector<std::vector<double>>& evaluation_histories) {
    const double G = static_cast<double>(evaluation_histories.size());
    
    // Get the total number of moves across all games
    double acc = 0.0;
    for (auto history : evaluation_histories) {
        const double M_g = history.size() - 1;
        acc += M_g;
    }

    // Set M_pref to the average number of moves
    const double M_pref = acc / G;

    // Sum up duration values for all games
    acc = 0.0;
    for (auto history : evaluation_histories) {
        const double M_g = history.size() - 1;
        acc += std::abs(M_pref - M_g) / M_pref;
    }

    // Return the average, or 1 if this value is greater than 1
    return std::min(1.0, acc / G);
}

/**
 * @brief Function to compute the lead change quality criterion.
 * @details Calculates the average lead change, measured as the number of times the evaluation
 * (taken with respect to player 0) changes sign.
 * @param evaluation_histories A vector of vectors of doubles containing evaluations with respect to player 0 for all games.
 * @return A double in the interval [0, 1] representing the average lead change. 0 indicates no lead changes, while 1 indicates
 * a lead change on every turn.
 */
double compute_lead_change(std::vector<std::vector<double>>& evaluation_histories) {
    const double G = static_cast<double>(evaluation_histories.size());
    
    // Calculate the total number of lead changes across all games
    double acc = 0.0;
    for (auto history : evaluation_histories) {
        int num_lead_changes = 0;

        // Start from the second move, since there will always be a new leader after the first move
        for (auto it = history.begin() + 2; it != history.end(); ++it) {
            num_lead_changes += (std::signbit(*it) != std::signbit(*(it-1))) ? 1 : 0;
        }
        const double M_g = history.size() - 1; 
        acc += static_cast<double>(num_lead_changes) / (M_g - 1);
    }
    
    // Return the average
    return acc / G;
}

/**
 * @brief Function to compute the uncertainty (late) quality criterion.
 * @details Calculates the late uncertainty, measured as an approximation of the area between the curve of the absolute value of
 * the evaluations and the curve (line) extending from (0, 0) to (M_g - 1, 1). Intuitively, this captures the size of the lead
 * difference with respect to either player (since the evaluation is zero-sum, taking the absolute value gives us this lead
 * difference) over time. A game where one player has a large lead for most of the game would result in a large area below
 * the lead curve and above the line from (0, 0) to (M_g - 1, 1), corresponding to a negative uncertainty (i.e., high certainty),
 * while a game where no player has a lead lead for most of the game would result in a large area below the line from (0, 0) to
 * (M_g - 1, 1) and above the lead curve, corresponding to a positive uncertainty (i.e., high uncertainty). 0.5 is added to the
 * final result so that it falls between 0 and 1, like the other statistics. This version of the uncertainty formula weighs the
 * late game more heavily.
 * @param evaluation_histories A vector of vectors of doubles containing evaluations with respect to player 0 for all games.
 * @return A double in the interval [0, 1] representing the average late uncertainty. 0 indicates no uncertainty, as in a game
 * where the absolute difference in evaluation is 1 after every move, while 1 indicates maximum uncertainty, as in a game where 
 * the difference in evaluation is 0 after every move.
 */
double compute_late_uncertainty(std::vector<std::vector<double>>& evaluation_histories) {
    // Number of samples (rectangles) used to approximate the area
    const int S = 100;
    const double G = static_cast<double>(evaluation_histories.size());

    // Calculate the sum of the samples across all games
    double samples_acc = 0.0;
    for (int s = 0; s < S; ++s) {
        // t represents a time point in the game and falls in the interval [0, 1]
        const double t = static_cast<double>(s) / static_cast<double>(S-1);

        // Sum up the evaluation difference at each time point
        double games_acc = 0.0;
        for (auto history : evaluation_histories) {
            const double M_g = static_cast<double>(history.size());     // no minus one here, as we want to include the final evaluation of 1.0 or -1.0
            const double tM_g = t * M_g;    // corresponds to a fractional move
            const int floor_index = static_cast<int>(tM_g);
            const int ceil_index = floor_index + 1;
            const double move_fraction = tM_g - static_cast<double>(floor_index);   // fractional part of the move
            const double floor_eval = std::abs(history[floor_index]);
            const double ceil_eval = std::abs(history[ceil_index]);
            const double fractional_eval = floor_eval + (ceil_eval - floor_eval) * move_fraction;   // evaluation of the fractional move
            games_acc += fractional_eval;
        }
        samples_acc += std::min(1.0, t - (games_acc / G));
    }

    // Return 0.5 plus the average to get result in the interval [0, 1]
    return 0.5 + (samples_acc / S);
}

/**
 * @brief Gets inputs from the user needed to run the trial.
 * @details Gets the number of simulations to run, whether to use the evaluation function, and which agents to use.
 * The user is re-prompted for a new line of input if any errors are present in the original input.
 * @return A vector of ints containing the user inputs satisfying: inputs.size() in [4, 7], inputs[0] in [1, 100,000], inputs[2 .. inputs.size()-1] each in [0, 23]. 
 */
std::vector<int> get_inputs() {
    // Prompt the user
    std::cout << "\nWelcome to the Qwixx analyzer tool. The available agents are\n\n"
              << "0: Random\n"
              << "1-10: GreedyNSkip (1 <= N <= 10)\n"
              << "11-20: GreedyNSkipImproved (1 <= N <= 10)\n"
              << "21: RushLocks\n"
              << "22: Computational\n"
              << "23: Human\n"
              << "\nPlease input the number of simulations, followed by a 1 if you would like to use the evaluation function (0 otherwise),\n\tfollowed by a sequence of 2 to 5 numbers corresponding to the above numbers for each agent.\n"
              << "Example: 10000 1 0 3 for 10000 simulations of Random vs. Greedy3Skip, where Random is evaluated.\n"
              << "Note that the evaluation function is only meaningful for 2 players, and will be disabled at higher player counts.\n\n";
    
    std::string line;
    std::vector<int> inputs;
    const size_t min_inputs = 4;
    const size_t max_inputs = 7;
    const int max_simulations = 100'000;
    const int agent_range_start = 0;
    const int agent_range_end = 23;
    
    // We will break out of this loop if there are no errors with the input
    while (true) {
        std::getline(std::cin, line);
        std::istringstream iss(line);
        inputs = {};
        int next = 0;
        while (!iss.eof()) {
            iss >> next;

            // Unspecified error -- probably a non-numeric value
            if (iss.fail()) {
                std::cout << "Error parsing input. All inputs should be numeric and not too large. Please retry.\n";
                goto retry;
            }

            inputs.push_back(next);
        }

        if (inputs.size() < min_inputs) {
            std::cout << "Too few inputs. Need at least 4: number of simulations, use of evaluation function, and at least two agents. Please retry.\n";
            goto retry;
        }

        if (inputs.size() > max_inputs) {
            std::cout << "Too many inputs. There can be at most 7: number of simulations, use of evaluation function, and at most five agents. Please retry.\n";
            goto retry;
        }
        
        if (inputs[0] < 1 || inputs[0] > max_simulations) {
            std::cout << "Invalid number of simulations: should be a number between 1 and 100,000. Please retry.\n";
            goto retry;
        }

        for (auto it = inputs.begin() + 2; it != inputs.end(); ++it) {
            if (*it < agent_range_start || *it > agent_range_end) {
                std::cout << "At least one agent number is invalid: valid agent numbers are " << agent_range_start << " through " << agent_range_end << ". Please retry.\n";
                goto retry;
            }
        }
        
        // All checks passed -- break out of loop
        break;

        // Some error encountered -- restart loop
        retry:
            continue;
    }

    // This is not classified as a user input error, so it is handled separately
    if (inputs.size() > 4 && inputs[1] != 0) {
        std::cout << "The evaluation function does not currently support more than 2 players. It will be disabled for this trial.\n";
        inputs[1] = 0;

    }

    return inputs;
}

/**
 * @brief Gets the players of the game from the user inputs.
 * @details For each integer in the vector of inputs starting after the first two (which are for
 * the number of simulations and whether to use the evaluation function), create a tuple consisting
 * of a unique pointer to a newly-constructed agent corresponding to that integer, plus a string
 * representing the name of the agent.
 * @param inputs A read-only vector of ints containing the user inputs as collected by the get_inputs() function.
 * @return A vector of tuples of unique pointers to agents and strings representing the agent names.
 */
std::vector<std::tuple<std::unique_ptr<Agent>, std::string>> get_players(const std::vector<int>& inputs) {
    std::vector<std::tuple<std::unique_ptr<Agent>, std::string>> players;
    for (auto it = inputs.begin() + 2; it != inputs.end(); ++it) {    
        // It would be preferable to directly map these values to the desired constructor, but I
        // don't know how to do that. This is fine for a small number of agents, though.
        if (*it == 0) {
            players.push_back(std::tuple(std::make_unique<Random>(), "Random"));
        }
        else if (*it >= 1 && *it <= 10) {
            std::string name = "Greedy" + std::to_string(*it) + "Skip";
            players.push_back(std::tuple(std::make_unique<Greedy>(*it), name));
        }
        else if (*it >= 11 && *it <= 20) {
            std::string name = "Greedy" + std::to_string((*it - 10)) + "SkipImproved";
            players.push_back(std::tuple(std::make_unique<GreedyImproved>(*it - 10), name));
        }
        else if (*it == 21) {
            players.push_back(std::tuple(std::make_unique<RushLocks>(), "RushLocks"));
        }
        else if (*it == 22) {
            players.push_back(std::tuple(std::make_unique<Computational>(), "Computational"));
        }
        else if (*it == 23) {
            players.push_back(std::tuple(std::make_unique<Human>(), "Human"));
        }
        else {
            players.push_back(std::tuple(std::make_unique<Random>(), "Random"));
        }
    }

    return players;
}

/**
 * @brief Checks if a human player is present in the game.
 * @param inputs A read-only vector of ints containing the user inputs as collected by the get_inputs() function.
 * @param human_id An int corresponding to the human agent.
 * @return A bool which is true if a human player was found, else false.
 */
bool is_human_active(const std::vector<int>& inputs, int human_id) {
    return std::find(inputs.begin() + 2, inputs.end(), human_id) != inputs.end();
}