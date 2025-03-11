#include <iostream>

#include "game.hpp"

int main() {
    for (int i = 0; i < 100000; ++i) {
        Game g = Game(static_cast<size_t>(4));
        std::unique_ptr<GameData> stats = g.run();
    }

    return 0;
}