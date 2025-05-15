#include <chrono>
#include <iostream>

#include "agent.hpp"
#include "game.hpp"

int main() {
    auto start = std::chrono::high_resolution_clock::now();

    int num_random_wins = 0;

    std::vector<std::unique_ptr<Agent>> players;
    players.push_back(std::make_unique<RandomAgent>());
    players.push_back(std::make_unique<GreedyAgent>(2));

    std::vector<Agent*> players_raw;
    for (const auto& player : players) {
        players_raw.push_back(player.get());
    }

    for (int i = 0; i < 1'000; ++i) {
        std::cout << "GAME " << (i + 1) << "\n\n";

        // TOOD: should consider reusing this object rather than repeatedly calling its constructor
        Game game = Game(players_raw);
        std::unique_ptr<GameData> stats = game.run();
        std::cout << "Winner: " << (stats.get()->winners[0] == 0 ? "random" : "greedy") << '\n'
                  << "Random score: " << stats.get()->final_score[0] << '\n'
                  << "Greedy score: " << stats.get()->final_score[1] << "\n\n";
                  //<< "Random scorepad:\n" << stats.get()->final_state.get()->scorepads[0] << '\n'
                  //<< "Greedy scorepad:\n" << stats.get()->final_state.get()->scorepads[1] << '\n';

        if (stats.get()->winners[0] == 0) {
            ++num_random_wins;
        }
    }

    std::cout << "Random wins: " << num_random_wins << '\n'
              << "Greedy wins: " << (1'000 - num_random_wins) << '\n';

    auto end = std::chrono::high_resolution_clock::now();
    std::chrono::duration<double> duration = end - start;

    std::cout << "Completed in " << duration.count() << " seconds\n";

    return 0;
}