#include "../src/game.hpp"

class ScorepadTester : Scorepad {
public:
    ScorepadTester() : Scorepad() {};
    void run_tests();
};

/*
class GameTester : Game {
public:
    GameTester(size_t num_players) : Game(num_players) {};
    void run_tests();
};
*/