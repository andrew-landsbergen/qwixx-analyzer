from mcts import mcts
import random

def random_policy():
    def fxn(pos):
        moves = pos.get_actions()
        if len(moves) > 0:
            return random.choice(moves)
        else:
            return None
    return fxn

def greedy_policy():    
    def fxn(pos):
        moves = pos.get_actions()
        if len(moves) > 0:
            # first priority is to get pieces out of the way of capture
            for move in moves:
                if pos.can_be_jumped(move, False):
                    return move
            
            # orders moves by furthest down the board (assumes control of the down player)
            for i in range(len(moves) - 1):
                min_pos = float('inf')
                min_pos_ind = None
                for j in range(i, len(moves)):
                    if moves[j].to_pos[1] < min_pos:
                        min_pos = moves[j].to_pos[1]
                        min_pos_ind = j
                tmp = moves[i]
                moves[i] = moves[min_pos_ind]
                moves[min_pos_ind] = tmp 

            # avoids moves that result in a piece that can be jumped
            for move in moves:
                if not pos.can_be_jumped(move, True):
                    return move

            # if no such move, move the pawn with the least progress
            for i in range(len(moves) - 1, 0, -1):
                if move.piece.type == 'P':
                    return move

            # else make a random move
            return random.choice(moves)
        else:
            return None
    return fxn

def mcts_policy(cpu_time):
    def fxn(pos):
        return mcts(pos, cpu_time * 1e9)
    return fxn   