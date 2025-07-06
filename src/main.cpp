#include <chrono>
#include <iostream>
#include <sstream>
#include <tuple>

#include "agent.hpp"
#include "game.hpp"

int main() {
    std::cout << "\nWelcome to the Qwixx analyzer tool. The available agents are\n\n"
              << "0: Random\n"
              << "1-10: GreedyNSkip (1 <= N <= 10)\n"
              << "11-20: GreedyNSkipImproved (1 <= N <= 10)\n"
              << "21: RushLocks\n"
              << "22: Computational\n"
              << "23: Human\n"
              << "\nPlease input the number of simulations, followed by a 1 if you would like to use the evaluation function (0 otherwise), followed by a sequence of 2 to 5 numbers corresponding to the above numbers for each agent.\n"
              << "Example: 10000 1 0 3 for 10000 simulations of Random vs. Greedy3Skip, where Random is evaluated.\n"
              << "Note that the evaluation function is only meaningful for 2 players, and will be disabled at higher player counts.\n\n";
    
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
    int num_agents = 0;
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

    int num_p1_wins = 0;
    int num_p2_wins = 0;
    int p1_total_score = 0;
    int p2_total_score = 0;

    //int p1_mutation_score = 0;
    //int p2_mutation_score = 0;

    int num_turns = 0;
    int min_turns = std::numeric_limits<int>::max();
    int max_turns = std::numeric_limits<int>::min();
    //int games_per_mutation = 500;
    
    std::array<double, 24> p0_evaluation_by_turn{};
    int num_games_tracked = 0;

    for (int i = 0; i < num_simulations; ++i) {
        /*if (i % games_per_mutation == 0) {
            bool final_mutation = (i + games_per_mutation >= num_simulations) ? true : false;
            player_ptrs[0]->mutate(static_cast<double>(p1_mutation_score) / static_cast<double>(games_per_mutation), final_mutation);
            player_ptrs[1]->mutate(static_cast<double>(p2_mutation_score) / static_cast<double>(games_per_mutation), final_mutation);
            p1_mutation_score = 0;
            p2_mutation_score = 0;
        }*/
        
        std::cout << "GAME " << (i + 1) << "\n\n";

        // TOOD: should consider reusing this object rather than repeatedly calling its constructor
        Game game = Game(player_ptrs, human_active, (static_cast<bool>(use_evaluation) && players.size() == 2));
        std::unique_ptr<GameData> stats = game.run();

        // TODO: make this loop over all players, track win counts for each player, and track ties
        std::cout << "Winner: " << (std::get<1>(players[stats.get()->winners[0]])) << '\n'
                  << std::get<1>(players[0]) << " score: " << stats.get()->final_score[0] << '\n'
                  << std::get<1>(players[1]) << " score: " << stats.get()->final_score[1] << "\n\n";

        if (stats.get()->winners[0] == 0) {
            ++num_p1_wins;
        }
        else if (stats.get()->winners[0] == 1) {
            ++num_p2_wins;
        }

        p1_total_score += stats.get()->final_score[0];
        p2_total_score += stats.get()->final_score[1];

        if (stats.get()->winners[0] == 0 && stats.get()->p0_evaluation_history.size() == 23) {
            for (size_t i = 0; i < 23; ++i) {
                p0_evaluation_by_turn[i] += stats.get()->p0_evaluation_history[i];
            }
            num_games_tracked += 1;
        }

        min_turns = std::min(min_turns, stats.get()->num_turns);
        max_turns = std::max(max_turns, stats.get()->num_turns);

        //p1_mutation_score += stats.get()->final_score[0];
        //p2_mutation_score += stats.get()->final_score[1];

        //num_turns += stats.get()->num_turns;
    }

    std::cout << std::get<1>(players[0]) << " wins: " << num_p1_wins << '\n'
              << std::get<1>(players[1]) << " wins: " << num_p2_wins << "\n\n";

    std::cout << std::get<1>(players[0]) << " average score: " << (p1_total_score / num_simulations) << '\n'
              << std::get<1>(players[1]) << " average score: " << (p2_total_score / num_simulations) << '\n';

    std::cout << "Average number of turns: " << static_cast<double>(num_turns) / static_cast<double>(num_simulations) << '\n';
    std::cout << "Maximum number of turns: " << max_turns << '\n';
    std::cout << "Minimum number of turns: " << min_turns << '\n';

    if (num_games_tracked > 0) {
        for (size_t i = 0; i < 23; ++i) {
            p0_evaluation_by_turn[i] /= static_cast<double>(num_games_tracked);
        }
        p0_evaluation_by_turn[23] = 1.0;
    }

    std::cout << "Player 0 evaluation by turn (" << num_games_tracked << " won games with exactly 23 turns):\n";
    for (size_t i = 0; i < 24; ++i) {
        std::cout << i << ", " << p0_evaluation_by_turn[i] << '\n';
    }

    auto end = std::chrono::high_resolution_clock::now();
    std::chrono::duration<double> duration = end - start;

    std::cout << "Completed in " << duration.count() << " seconds\n";

    return 0;
}