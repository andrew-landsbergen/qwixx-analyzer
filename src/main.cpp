#include <chrono>
#include <cmath>
#include <iostream>
#include <sstream>
#include <tuple>

#include "agent.hpp"
#include "game.hpp"
#include "rng.hpp"

double compute_duration(std::vector<std::vector<double>>& evaluation_histories) {
    const double G = static_cast<double>(evaluation_histories.size());
    
    double acc = 0.0;
    for (auto history : evaluation_histories) {
        const double M_g = history.size() - 1;
        acc += M_g;
    }

    const double M_pref = acc / G;

    acc = 0.0;
    for (auto history : evaluation_histories) {
        const double M_g = history.size() - 1;
        acc += std::abs(M_pref - M_g) / M_pref;
    }

    return acc / G;
}

double compute_lead_change(std::vector<std::vector<double>>& evaluation_histories) {
    const double G = static_cast<double>(evaluation_histories.size());
    
    double acc = 0.0;
    for (auto history : evaluation_histories) {
        int num_lead_changes = 0;
        for (auto it = history.begin() + 2; it != history.end(); ++it) {
            num_lead_changes += (std::signbit(*it) != std::signbit(*(it-1))) ? 1 : 0;
        }
        const double M_g = history.size() - 1; 
        acc += static_cast<double>(num_lead_changes) / (M_g - 1);
    }
    
    return acc / G;
}

double compute_late_uncertainty(std::vector<std::vector<double>>& evaluation_histories) {
    const int S = 100;
    const double G = static_cast<double>(evaluation_histories.size());

    double samples_acc = 0.0;
    for (int s = 0; s < S; ++s) {
        const double t = static_cast<double>(s) / static_cast<double>(S-1);
        double games_acc = 0.0;
        for (auto history : evaluation_histories) {
            const double M_g = static_cast<double>(history.size());     // no minus one here, as we want to include the final evaluation of 1.0 or -1.0
            const double tM_g = t * M_g;
            const int floor_index = static_cast<int>(tM_g);
            const int ceil_index = floor_index + 1;
            const double move_fraction = tM_g - static_cast<double>(floor_index);
            const double floor_eval = std::abs(history[floor_index]);
            const double ceil_eval = std::abs(history[ceil_index]);
            const double fractional_eval = floor_eval + (ceil_eval - floor_eval) * move_fraction;
            games_acc += fractional_eval;
        }
        samples_acc += std::min(1.0, t - (games_acc / G));
    }

    return samples_acc / S;
}

int main() {
    /*std::cout << "\nWelcome to the Qwixx analyzer tool. The available agents are\n\n"
              << "0: Random\n"
              << "1-10: GreedyNSkip (1 <= N <= 10)\n"
              << "11-20: GreedyNSkipImproved (1 <= N <= 10)\n"
              << "21: RushLocks\n"
              << "22: Computational\n"
              << "23: Human\n"
              << "\nPlease input the number of simulations, followed by a 1 if you would like to use the evaluation function (0 otherwise),\n\tfollowed by a sequence of 2 to 5 numbers corresponding to the above numbers for each agent.\n"
              << "Example: 10000 1 0 3 for 10000 simulations of Random vs. Greedy3Skip, where Random is evaluated.\n"
              << "Note that the evaluation function is only meaningful for 2 players, and will be disabled at higher player counts.\n\n";
    */
    // TOOD: validate input
    std::string line;
    std::getline(std::cin, line);
    std::istringstream iss(line);

    int num_simulations = 1;
    iss >> num_simulations;
    
    int use_evaluation = 0;
    iss >> use_evaluation;

    std::vector<std::tuple<std::unique_ptr<Agent>, std::string>> players;
    int agent_selection = 0;
    size_t num_agents = 0;
    bool human_active = false;
    while (num_agents < 5 && iss >> agent_selection) {
        if (agent_selection == 0) {
            players.push_back(std::tuple(std::make_unique<Random>(), "Random"));
        }
        else if (agent_selection >= 1 && agent_selection <= 10) {
            std::string name = "Greedy" + std::to_string(agent_selection) + "Skip";
            players.push_back(std::tuple(std::make_unique<Greedy>(agent_selection), name));
        }
        else if (agent_selection >= 11 && agent_selection <= 20) {
            std::string name = "Greedy" + std::to_string((agent_selection - 10)) + "SkipImproved";
            players.push_back(std::tuple(std::make_unique<GreedyImproved>(agent_selection - 10), name));
        }
        else if (agent_selection == 21) {
            players.push_back(std::tuple(std::make_unique<RushLocks>(), "RushLocks"));
        }
        else if (agent_selection == 22) {
            players.push_back(std::tuple(std::make_unique<Computational>(), "Computational"));
        }
        else if (agent_selection == 23) {
            players.push_back(std::tuple(std::make_unique<Human>(), "Human"));
            human_active = true;
        }
        else {
            std::cout << agent_selection << " is not a valid agent number, skipping...\n";
            continue;
        }
        ++num_agents;
    }

    auto start = std::chrono::high_resolution_clock::now();

    std::vector<Agent*> player_ptrs;
    for (const auto& player : players) {
        player_ptrs.push_back(std::get<0>(player).get());
    }

    std::vector<double> num_wins_accum(num_agents);
    std::vector<int> score_accum(num_agents);
    int num_turns_accum = 0;
    int min_turns = std::numeric_limits<int>::max();
    int max_turns = std::numeric_limits<int>::min();

    std::vector<std::vector<double>> evaluation_histories(num_simulations);

    std::uniform_int_distribution<int> dist(0, num_simulations - 1);
    int random_trial = dist(rng());
    std::vector<double> saved_history;

    //int p1_mutation_score = 0;
    //int p2_mutation_score = 0;
    //int games_per_mutation = 500;
    
    //std::array<double, 24> p0_evaluation_by_turn{};
    //int num_games_tracked = 0;

    for (int i = 0; i < num_simulations; ++i) {
        /*if (i % games_per_mutation == 0) {
            bool final_mutation = (i + games_per_mutation >= num_simulations) ? true : false;
            player_ptrs[0]->mutate(static_cast<double>(p1_mutation_score) / static_cast<double>(games_per_mutation), final_mutation);
            player_ptrs[1]->mutate(static_cast<double>(p2_mutation_score) / static_cast<double>(games_per_mutation), final_mutation);
            p1_mutation_score = 0;
            p2_mutation_score = 0;
        }*/

        // TOOD: should consider reusing this object rather than repeatedly calling its constructor
        Game game = Game(player_ptrs, human_active, (static_cast<bool>(use_evaluation) && players.size() == 2));
        std::unique_ptr<GameData> stats = game.run();

        if (i == random_trial) {
            saved_history = stats.get()->p0_evaluation_history;
        }

        for (size_t j = 0; j < num_agents; ++j) {
            if (std::find(stats.get()->winners.begin(), stats.get()->winners.end(), j) != stats.get()->winners.end()) {
                num_wins_accum[j] += 1.0 / static_cast<double>(stats.get()->winners.size());
            }

            score_accum[j] += stats.get()->final_score[j];
        }

        evaluation_histories[i] = std::move(stats.get()->p0_evaluation_history);

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

    for (size_t i = 0; i < num_agents; ++i) {
        std::cout << "Player " << i << " (" << std::get<1>(players[i]) << ") win rate: " << num_wins_accum[i] / static_cast<double>(num_simulations) << '\n';
        std::cout << "Player " << i << " (" << std::get<1>(players[i]) << ") average score: " << static_cast<double>(score_accum[i]) / static_cast<double>(num_simulations) << '\n';
    }

    std::cout << "Average number of turns: " << static_cast<double>(num_turns_accum) / static_cast<double>(num_simulations) << '\n';
    std::cout << "Maximum number of turns: " << max_turns << '\n';
    std::cout << "Minimum number of turns: " << min_turns << '\n';

    if (static_cast<bool>(use_evaluation)) {
        const double duration_stat = compute_duration(evaluation_histories);
        const double lead_change_stat = compute_lead_change(evaluation_histories);
        const double late_uncertainty_stat = compute_late_uncertainty(evaluation_histories);

        std::cout << "Duration statistic: " << duration_stat << '\n';
        std::cout << "Lead change statistic: " << lead_change_stat << '\n';
        std::cout << "Uncertainty (late) statistic: " << late_uncertainty_stat << '\n';

        std::cout << "Randomly selected evaluation history (trial #" << random_trial << "):\n";
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

    auto end = std::chrono::high_resolution_clock::now();
    std::chrono::duration<double> duration = end - start;

    //std::cout << "Completed in " << duration.count() << " seconds\n";

    return 0;
}