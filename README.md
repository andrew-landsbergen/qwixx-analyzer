This is my final project submission for CPSC 474. I will use this space to describe what the program I have submitted can currently do, as well as the things that
still need work.

# Running the program
    Run `make Checkers`, followed by `./Checkers` to run the default program. See "What the program can do" for options that can be passed in.

# The game
    In checkers, the goal is to make the last move. Your opponent will not be able to move if he either has no more pieces or all of his pieces are prevented from moving. Pieces move diagonally and can either be pawns or kings. Pawns can only move up the board, while kings can move backwards. Pawns turn into kings when they reach the end of the board. To capture a piece, one jumps over it (still diagonally) into an empty space behind that piece. All jumps must be taken, and when an opportunity for a multiple jump exists (jumping again after a jump has been made), it can and must be taken. 

# What the program can do
    The program has two agents play a representation of checkers. The first agent uses an MCTS algorithm, and the second uses a basic greedy algorithm. The MCTS
    algorithm is exactly the same as that used for homework 4. The greedy algorithm runs a few checks: first it moves pieces out of the way if they are about to be
    captured, then it moves the piece furthest up the board such that that move would not immediately result in capture, then it moves the pawn with the least
    progress, and if none of those moves are possible, it makes a random move.

    Arguments can be passed into the program to configure it. Here are the current options:
        [--games n]: the number of games to play
        [--size n]: the dimensions of the checkerboard (min 3x3, max 12x12)
        [--draw_limit n]: the maximum number of turns that may pass since the last capture and the last pawn move before a draw is declared
        [--display]: draw the checkerboard at the end of the game
        [--displayall]: draw the checkerboard after every move
        [--move_delay t]: the amount of time to wait between displaying the checkerboard after every move
        [--time t]: the amount of time the MCTS agent is given to make its move

# Things that still need work
    I will preface this by saying that I don't consider this project to be finished and am simply turning in what I have, as there are only a few hours
    remaining until the deadline and I am tired. I spent at least 16 hours working on it. The main reason it took me so long is that several complicated 
    bugs came up that I did not understand how to fix, and I had to rework the structure of my code multiple times to even understand what was happening.
    As it turned out, the core problem lay in the fact that unexpected modifications to state were happening during the simulation phase of MCTS. In 
    particular, when using the `make_move` function with a `Move` object, that `Move` object would use `Piece` objects belonging to the old `State` object
    instead of the new one.   

    I initially wanted to make some improvements to the MCTS algorithm as discussed in class, such as MAST/PAST or UCB1. Due to running out of time, I have
    not implemented these. If I had had the time, I likely would have started with implementing MAST by dividing moves into the following categories: moves
    with pawns, moves with kings, single jumps, multiple jumps, moves to left or right edges, kinging moves, and perhaps some others. 

    The greedy algorithm could also be a bit better, as it tends to meander about with its kings like the random agent. More positional play, such as trying
    to get to the edges and taking away space from the opponent would result in better performance, although with such improvements it may no longer be 
    accurate to call the algorithm purely greedy.

# Results
    The MCTS agent seems to perform decently against the greedy agent. Running 50 games on an 8x8 checkerboard with a draw limit of 10 and time limit of 0.1 seconds resulted in an average win rate of 83%. 