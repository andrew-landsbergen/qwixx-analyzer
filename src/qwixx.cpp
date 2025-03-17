#include <chrono>
#include <iostream>

#include "game.hpp"

int main() {
    auto start = std::chrono::high_resolution_clock::now();

    for (int i = 0; i < 100'000; ++i) {
        // TOOD: should consider reusing this object rather than repeatedly calling its constructor
        Game game = Game(4);
        std::unique_ptr<GameData> stats = game.run();
        (void) stats;   // discard for now
    }

    auto end = std::chrono::high_resolution_clock::now();
    std::chrono::duration<double> duration = end - start;

    std::cout << "Completed in " << duration.count() << " seconds\n";

    return 0;
}