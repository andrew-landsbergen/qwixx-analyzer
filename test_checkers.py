import sys, time
import checkers, policy

# see README for a description of the game and other comments

if __name__ == "__main__":
	games_left = 1
	size = 8
	draw_limit = 40
	display = False
	displayall = False
	move_delay = 1
	mcts_time = 0.25

	args = sys.argv
	if len(args) > 1:
		for i in range(1, len(args)):
			if args[i] == "--games":
				try:
					games_left = int(args[i + 1])
				except:
					print("The --games argument should be followed by an integer representing the number of games to play")
					sys.exit(0)
			elif args[i] == "--size":
				try:
					size = int(args[i + 1])
					if size > 12:
						print("Maximum size of 12")
						sys.exit(0)
					elif size < 3:
						print("Minimum size of 3")
						sys.exit(0)
				except:
					print("The --size argument should be followed by an integer n representing the n x n board dimensions")
					sys.exit(0)
			elif args[i] == "--draw_limit":
				try:
					draw_limit = int(args[i + 1])
					if draw_limit > 60:
						print("Maximum draw limit of 60")
						sys.exit(0)
					elif draw_limit <= 0:
						print("Minimum draw limit of 1")
						sys.exit(0)
				except:
					print("The --draw_limit argument should be followed by an integer representing the maximum number of turns " +
						  "since last capture and last pawn move until a draw is declared")
					sys.exit(0)
			elif args[i] == "--display":
				display = True
			elif args[i] == "--displayall":
				displayall = True
			elif args[i] == "--move_delay":
				try:
					displayall = True
					move_delay = float(args[i + 1])
				except:
					print("The --move_delay argument should be followed by a float representing the number of seconds to wait " +
						   "before drawing the next move")
					sys.exit(0)
			elif args[i] == "--time":
				try:
					mcts_time = float(args[i + 1])
				except:
					print("The --time argument should be followed by a float representing the amount of time the MCTS agent " +
						  "uses to move")
					sys.exit(0)

	p0_wins = 0
	total_games = games_left
	game = checkers.Game(size, draw_limit)
	p0_policy = policy.mcts_policy(mcts_time)
	p1_policy = policy.greedy_policy()
	while games_left > 0:
		pos = game.initial_state()
		while pos.result is None:
			if pos.actor() == 0:
				pos.make_move(p0_policy(pos))
			else:
				pos.make_move(p1_policy(pos))
			
			if displayall:
				pos.display()
				time.sleep(move_delay)

		if display:
			pos.display()

		p0_wins += pos.result
		games_left -= 1
	
	print("Average win rate:", p0_wins / total_games)
		
