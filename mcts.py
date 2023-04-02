import time, random
from math import log, sqrt

# index 0: [reward, value]
# index 1: game state associated with node
# index 2: player whose turn it is
# index 3: parent
# index 4: list of children
def create_node(state, player, parent):
    return [[0, 0], state, player, parent, []]     

def add_child(parent, state, player):
    parent[4].append(create_node(state, player, parent))

def update_node_value(node, reward):
    node[0][0] += reward
    node[0][1] += 1

# returns rewards, visits
def get_node_value(node):
    return node[0][0], node[0][1]

def get_node_state(node):
    return node[1]

def get_node_player(node):
    return node[2]

def get_node_parent(node):
    return node[3]

def get_node_children(node):
    return node[4]

# assumes nodes is not None
def choose_node(nodes, player):
    if player == 0:
        ucb_max_val = float('-inf')
        ucb_max_node = None
        for node in nodes:
            r, n = get_node_value(node)
            if n == 0:
                return node
            else:
                _, T = get_node_value(get_node_parent(node))
                ucb = r/n + sqrt(2 * log(T) / n) 
                if ucb > ucb_max_val:
                    ucb_max_val = ucb
                    ucb_max_node = node
        return ucb_max_node
    else:
        ucb_min_val = float('inf')
        ucb_min_node = None
        for node in nodes:
            r, n = get_node_value(node)
            if n == 0:
                return node
            else:
                _, T = get_node_value(get_node_parent(node))
                ucb = r/n - sqrt(2 * log(T) / n)
                if ucb < ucb_min_val:
                    ucb_min_val = ucb
                    ucb_min_node = node
        return ucb_min_node
    
def traverse(root):
    curr_node = root
    curr_player = get_node_player(root)
    curr_children = get_node_children(root)

    # stop at leaf
    while len(curr_children) > 0:
        curr_node = choose_node(curr_children, curr_player)       
        curr_children = get_node_children(curr_node)
        curr_player = get_node_player(curr_node)
    return curr_node

def expand(leaf):
    expanded = False
    pos = get_node_state(leaf)
    _, visits = get_node_value(leaf)
    moves = pos.get_actions()

    # add children if the position is not terminal and the node has not been visited
    if not pos.is_terminal() and visits != 0:
        for move in moves:
            succ = pos.successor(move)
            add_child(leaf, succ, succ.actor())
        expanded = True
    return expanded

def simulate(leaf, expanded):
    if expanded:
        simulate(get_node_children(leaf)[0], False)
    
    pos = get_node_state(leaf)
    succ = pos

    # make random moves until the game ends
    while not succ.is_terminal():
        succ = succ.successor(random.choice(succ.get_actions()))
    return succ.payoff(), leaf

def update(result, sim_start, root):
    curr = sim_start
    while (curr != None):
        update_node_value(curr, result)
        curr = get_node_parent(curr)

def mcts_policy(cpu_time):
    def fxn(pos):
        return mcts(pos, cpu_time * 1e9)
    return fxn   

def mcts(pos, t):
    time_start = time.time_ns()
    player = pos.actor()
    root = create_node(pos, player, None)
    expand(root)    # create second level of tree
    while (time.time_ns() - time_start <= t):
        # traverses tree from root to leaf, using UCB to pick child nodes
        leaf = traverse(root)
        
        # expands leaf if expandable
        expanded = expand(leaf)

        # simulates random play from leaf, or first child of leaf if newly expanded
        result, sim_start = simulate(leaf, expanded)
        
        # backprop result from start of sim to root
        update(result, sim_start, root)
    
    # select best move
    moves = pos.get_actions()
    if len(moves) == 0:
        return None
    children = get_node_children(root)
    best_move = moves[0]   
    if player == 0:
        # max player
        max_mean_reward = float('-inf')
        for move, node in zip(moves, children):    
            r, n = get_node_value(node)
            if n == 0:
                mean_reward = 0
            else:
                mean_reward = r/n
            if mean_reward > max_mean_reward:
                max_mean_reward = mean_reward
                best_move = move
    else:
        # min player
        min_mean_reward = float('inf')
        for move, node in zip(moves, children):
            r, n = get_node_value(node)
            if n == 0:
                mean_reward = 0
            else:
                mean_reward = r/n
            if mean_reward < min_mean_reward:
                min_mean_reward = mean_reward
                best_move = move
    return best_move        # can be None
    


