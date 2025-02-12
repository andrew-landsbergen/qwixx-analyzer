#include <iostream>

class Game {
private:
    int numPlayers;
public:
    Game(int n) : numPlayers(n) {
        if (numPlayers >= 2) {
            std::cout << "Game started." << '\n';
        }
    }
};