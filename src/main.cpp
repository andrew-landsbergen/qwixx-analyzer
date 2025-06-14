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
              << "\nPlease input the number of simulations, followed by a sequence of 2 to 5 numbers corresponding to the above numbers for each agent.\n"
              << "Example: 10000 0 4 15 for 10000 simulations of Random vs. Greedy4Skip vs. GreedySkipLowProbability5Skip.\n\n";
    
    // TOOD: validate input
    std::string line;
    std::getline(std::cin, line);
    std::istringstream iss(line);

    int num_simulations = 1;
    iss >> num_simulations;    

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
            players.push_back(std::tuple(std::make_unique<RushLocks>(50), "RushLocks"));
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

    for (int i = 0; i < num_simulations; ++i) {
        std::cout << "GAME " << (i + 1) << "\n\n";

        // TOOD: should consider reusing this object rather than repeatedly calling its constructor
        Game game = Game(player_ptrs, human_active);
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
    }

    std::cout << std::get<1>(players[0]) << " wins: " << num_p1_wins << '\n'
              << std::get<1>(players[1]) << " wins: " << num_p2_wins << "\n\n";

    std::cout << std::get<1>(players[0]) << " average score: " << (p1_total_score / num_simulations) << '\n'
              << std::get<1>(players[1]) << " average score: " << (p2_total_score / num_simulations) << '\n';

    auto end = std::chrono::high_resolution_clock::now();
    std::chrono::duration<double> duration = end - start;

    std::cout << "Completed in " << duration.count() << " seconds\n";

    return 0;
}