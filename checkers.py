from math import ceil, floor
import random

class Game:
	def __init__(self, size, draw_limit):
		""" Creates a checkers game of the given n x n board size, and where the maximum number of turns
			that may pass before a draw is declared is given by draw_limit.

			size -- a positive integer greater than or equal to 3
			draw_limit -- a positive integer less than or equal to 60
		"""
		self.size = size
		self.draw_limit = draw_limit
		
	def initial_state(self):
		""" Creates an initial game state. 
		"""
		return Game.State(self, None, 0, 0, random.choice([0, 1]))

	class State:
		def __init__(self, game, old_board, turns_since_capture, turns_since_pawn_move, current_player):
			""" Creates a checkers game state using the information passed in.

			game -- a Game object
			old_board -- a dict containing the board to copy over
			turns_since_capture -- a non-negative integer representing the number of turns since a piece was last captured
			turns_since_pawn_move -- a non-negative integer representing the number of turns since a pawn was last captured
			current_player -- the current player
			"""
			self.game = game
			new_board = dict()
			pieces_table = dict()

			# copy the board if it exists
			if old_board is not None:
				for i in range(self.game.size):
					for j in range(i % 2, self.game.size, 2):
						piece = old_board[(j, i)]
						if piece is not None:
							new_piece = Game.Piece(piece.owner, piece.type, piece.id, (j, i))
							new_board[(j, i)] = new_piece
							pieces_table[piece.id] = new_piece
						else:
							new_board[(j, i)] = None
			else:
				piece_id = 0
				for i in range(self.game.size):
					for j in range(i % 2, self.game.size, 2):
						if i < floor((self.game.size - 1) / 2):
							new_piece = Game.Piece(0, 'P', piece_id, (j, i))
							new_board[(j, i)] = new_piece
							pieces_table[piece_id] = new_piece
							piece_id += 1
						elif i > ceil((self.game.size - 1) / 2):
							new_piece = Game.Piece(1, 'P', piece_id, (j, i))
							new_board[(j, i)] = new_piece
							pieces_table[piece_id] = new_piece
							piece_id += 1
						else:
							new_board[(j, i)] = None

			self.board = new_board
			self.pieces_table = pieces_table
			self.turns_since_capture = turns_since_capture
			self.turns_since_pawn_move = turns_since_pawn_move
			self.current_player = current_player
			self.result = None

		def display(self):
			""" Draws a representation of the checkerboard. 
			"""
			board = ""
			for i in range(self.game.size):
				board += str(i) + "  " 	 
				for j in range(self.game.size):
					if (i + j) % 2 == 1:
						board += "  "
						continue

					piece = self.board[(j, i)]
					if piece is None:
						board += "_ "
					elif piece.owner == 0:
						if piece.type == 'P':
							board += "X "
						else:
							board += "* "
					else:
						if piece.type == 'P':
							board += "O "
						else:
							board += "+ "
				board += "\n"
			board += "   "
			for i in range(self.game.size):
				board += str(i) + " "
			board += "\n"
			
			print(board)

		def is_terminal(self):
			""" Returns True if the position is terminal, else False.
			"""
			self.get_actions()		# updates result if there are no moves
			return self.result is not None

		def payoff(self):
			""" Returns the result, or 0 if there is none.
			"""
			if self.result is None:
				return 0
			else:
				return self.result

		def actor(self):
			""" Returns the current player.
			"""
			return self.current_player

		def on_left_edge(self, piece):
			""" Returns True if the piece is on the left edge, else False.
				
				piece -- a Piece object
			"""
			if piece.pos[0] == 0:
				return True
			else:
				return False

		def on_right_edge(self, piece):
			""" Returns True if the piece is on the right edge, else False.
			
				piece -- a Piece object
			"""
			if piece.pos[0] == self.game.size - 1:
				return True
			else:
				return False

		def on_bottom_edge(self, piece):
			""" Returns True if the piece is on the bottom edge, else False.
			
				piece -- a Piece object
			"""
			if piece.pos[1] == 0:
				return True
			else:
				return False
	
		def on_top_edge(self, piece):
			""" Returns True if the piece is on the top edge, else False.
			
				piece -- a Piece object
			"""
			if piece.pos[1] == self.game.size - 1:
				return True
			else:
				return False 	 

		def make_move(self, move):
			""" Makes the given move, updating the game state accordingly.

				move -- a Move object
			"""

			# no move, game over
			if move is None:
				if self.current_player == 0:
					self.result = 0
				else:
					self.result = 1
				return

			# update board to reflect move
			self.board[move.from_pos] = None
			self.board[move.to_pos] = move.piece

			# update piece's position
			move.piece.pos = move.to_pos
			
			multiple_jump = False
			if move.captured_piece is not None:
				self.turns_since_capture = 0

				# remove captured piece from board and table of pieces
				self.board[move.captured_piece.pos] = None
				self.pieces_table.pop(move.captured_piece.id)
				
				# check for multiple jump
				jumps = self.find_jumps(move.piece)	
				if len(jumps) > 0:
					multiple_jump = True
			else:
				self.turns_since_capture += 1

			# check for kinging
			# N.B.: placing this after the multiple jump check means creating a new king after a jump
			# does not allow you to immediately double jump backwards with that new king, which
			# is my interpretation of the rules (since the jump started as a pawn)
			if move.piece.type == 'P':
				self.turns_since_pawn_move = 0

				# turn piece into king if it has reached the end
				if (move.piece.owner == 0 and self.on_top_edge(move.piece)) \
				  or (move.piece.owner == 1 and self.on_bottom_edge(move.piece)):
					move.piece.type = 'K'
			else:
				self.turns_since_pawn_move += 1
			
			# change player if not a multiple jump
			if not multiple_jump:
				self.current_player = 1 - self.current_player

			# check if game is drawn
			if self.turns_since_capture >= self.game.draw_limit and self.turns_since_pawn_move >= self.game.draw_limit:
				self.result = 1/2

		def can_be_jumped(self, move, check_move):
			""" Returns True if the piece to be moved can be jumped, either in its current position (if check_move is
				set to False) or in its position after the move (if check_move is set to True). 
				
				move -- a Move object
				check_move -- set to True to check the position after the move, False to check the current position
				"""
			moving_plr = move.piece.owner
			base_pos = move.piece.pos
			if check_move:
				base_pos = move.to_pos
			
			# safe if on an edge
			if base_pos[0] == 0 or base_pos[1] == 0 or base_pos[0] == self.game.size - 1 \
			  or base_pos[1] == self.game.size - 1:
				return False
			else:
				# checks top left, top right, bottom left, bottom right in that order
				for i in range(0, 4):
					# stuff to get the numbers right
					x_offset = 2 * (i % 2) - 1
					y_offset = 1
					pawn_required_owner = 1
					if i >= 2:
						y_offset = -1
						pawn_required_owner = 0

					# check neighboring space for piece
					piece_on_space = self.board[(base_pos[0] + x_offset, base_pos[1] + y_offset)]

					# piece is an enemy
					if piece_on_space is not None and piece_on_space.owner != moving_plr:
						# piece can jump in the required direction
						if piece_on_space.type == 'K' or piece_on_space.owner == pawn_required_owner:
							# piece has empty space to jump into
							piece_on_space = self.board[(base_pos[0] + -x_offset, base_pos[1] + -y_offset)]
							if piece_on_space is None or (check_move and piece_on_space is move.piece):
								return True
				# safe
				return False

		def find_jumps(self, piece):
			""" Finds any jumps that the given piece can make.
			
				piece -- a Piece object
			"""
			# different direction of movement depending on owner
			dir = 1
			if piece.owner == 1:
				dir = -1

			moves = []
			curr_pos = piece.pos

			# forwards moves
			if (piece.owner == 0 and not self.on_top_edge(piece)) or (piece.owner == 1 \
			  and not self.on_bottom_edge(piece)):
				# left
				if not self.on_left_edge(piece):
					piece_to_capture = self.board[(curr_pos[0] - 1, curr_pos[1] + 1 * dir)]
					if piece_to_capture is not None and piece_to_capture.owner != piece.owner \
					  and not self.on_left_edge(piece_to_capture) and ((piece.owner == 0 and not \
					  self.on_top_edge(piece_to_capture)) or (piece.owner == 1 and not \
					  self.on_bottom_edge(piece_to_capture))) and \
					  self.board[(curr_pos[0] - 2, curr_pos[1] + 2 * dir)] is None:
						moves.append(self.game.Move(piece, (curr_pos[0] - 2, curr_pos[1] + 2 * dir), piece_to_capture))
				# right
				if not self.on_right_edge(piece):
					piece_to_capture = self.board[(curr_pos[0] + 1, curr_pos[1] + 1 * dir)]
					if piece_to_capture is not None and piece_to_capture.owner != piece.owner \
					  and not self.on_right_edge(piece_to_capture) and ((piece.owner == 0 and not \
					  self.on_top_edge(piece_to_capture)) or (piece.owner == 1 and not \
					  self.on_bottom_edge(piece_to_capture))) and \
					  self.board[(curr_pos[0] + 2, curr_pos[1] + 2 * dir)] is None:
						moves.append(self.game.Move(piece, (curr_pos[0] + 2, curr_pos[1] + 2 * dir), piece_to_capture))
			# backwards moves for kings
			if piece.type == 'K':
				if (piece.owner == 0 and not self.on_bottom_edge(piece)) or (piece.owner == 1 \
				and not self.on_top_edge(piece)):
					# left
					if not self.on_left_edge(piece):
						piece_to_capture = self.board[(curr_pos[0] - 1, curr_pos[1] - 1 * dir)]
						if piece_to_capture is not None and piece_to_capture.owner != piece.owner \
						and not self.on_left_edge(piece_to_capture) and ((piece.owner == 0 and not \
						self.on_bottom_edge(piece_to_capture)) or (piece.owner == 1 and not \
						self.on_top_edge(piece_to_capture))) and \
						self.board[(curr_pos[0] - 2, curr_pos[1] - 2 * dir)] is None:
							moves.append(self.game.Move(piece, (curr_pos[0] - 2, curr_pos[1] - 2 * dir), piece_to_capture))
					# right
					if not self.on_right_edge(piece):
						piece_to_capture = self.board[(curr_pos[0] + 1, curr_pos[1] - 1 * dir)]
						if piece_to_capture is not None and piece_to_capture.owner != piece.owner \
						and not self.on_right_edge(piece_to_capture) and ((piece.owner == 0 and not \
						self.on_bottom_edge(piece_to_capture)) or (piece.owner == 1 and not \
						self.on_top_edge(piece_to_capture))) and \
						self.board[(curr_pos[0] + 2, curr_pos[1] - 2 * dir)] is None:
							moves.append(self.game.Move(piece, (curr_pos[0] + 2, curr_pos[1] - 2 * dir), piece_to_capture))
			return moves

		def find_regular_moves(self, piece):
			""" Finds any non-jump moves that the given piece can make.
			
				piece -- a Piece object
			"""
			# different direction of movement depending on owner
			dir = 1
			if piece.owner == 1:
				dir = -1

			moves = []
			curr_pos = piece.pos	

			# forwards moves
			if (piece.owner == 0 and not self.on_top_edge(piece)) or (piece.owner == 1 \
			and not self.on_bottom_edge(piece)):
				# left
				if not self.on_left_edge(piece):
					piece_in_space = self.board[(curr_pos[0] - 1, curr_pos[1] + 1 * dir)]
					if piece_in_space is None:
						moves.append(self.game.Move(piece, (curr_pos[0] - 1, curr_pos[1] + 1 * dir), None))
				# right
				if not self.on_right_edge(piece):
					piece_in_space = self.board[(curr_pos[0] + 1, curr_pos[1] + 1 * dir)]
					if piece_in_space is None:
						moves.append(self.game.Move(piece, (curr_pos[0] + 1, curr_pos[1] + 1 * dir), None))
			# backwards moves for kings
			if piece.type == 'K':
				if (piece.owner == 0 and not self.on_bottom_edge(piece)) or (piece.owner == 1 \
				and not self.on_top_edge(piece)):
					# left
					if not self.on_left_edge(piece):
						piece_in_space = self.board[(curr_pos[0] - 1, curr_pos[1] - 1 * dir)]
						if piece_in_space is None:
							moves.append(self.game.Move(piece, (curr_pos[0] - 1, curr_pos[1] - 1 * dir), None))
					# right
					if not self.on_right_edge(piece):
						piece_in_space = self.board[(curr_pos[0] + 1, curr_pos[1] - 1 * dir)]
						if piece_in_space is None:
							moves.append(self.game.Move(piece, (curr_pos[0] + 1, curr_pos[1] - 1 * dir), None))
			return moves

		def get_actions(self):
			""" Returns all possible moves the current player can make in this position.
			"""
			# get player's pieces
			pieces = []
			for key in self.pieces_table:
				piece = self.pieces_table[key]
				if piece.owner == self.current_player:
					pieces.append(piece)

			# look for jumps
			jumps = []
			for piece in pieces:
				jumps += self.find_jumps(piece)
			
			# must jump 
			if len(jumps) > 0:
				return jumps

			# look for regular moves
			regular_moves = []
			for piece in pieces:
				regular_moves += self.find_regular_moves(piece)
			
			# no jumps and no moves, so game is over
			if len(regular_moves) == 0:
				if self.current_player == 1:
					self.result = 1
				else:
					self.result = 0
			
			return regular_moves

		def successor(self, move):
			""" Creates a copy of the game state and makes the given move on it.

				move -- a Move object
			"""
			# copy state so as to not change old state when moving
			state_copy = Game.State(self.game, self.board, self.turns_since_capture, self.turns_since_pawn_move, self.current_player)
			
			# copy move so as to use the new state's pieces
			piece_equivalent = state_copy.pieces_table[move.piece.id]
			captured_piece_equivalent = None
			if move.captured_piece is not None:
				captured_piece_equivalent = state_copy.pieces_table[move.captured_piece.id]
			move_copy = Game.Move(piece_equivalent, move.to_pos, captured_piece_equivalent)

			# make the copied move on the copied state
			state_copy.make_move(move_copy)

			return state_copy

	class Piece:
		def __init__(self, owner, type, id, pos):
			""" Creates a checkers piece belonging to the given owner, of the given type, with the given ID for hashing, and
			at the given position.
			
			owner -- 0 or 1 for p0 or p1
			type -- 'P' or 'K' for pawn or king
			id -- an integer
			pos -- a tuple	
			"""
			self.owner = owner
			self.type = type
			self.id = id
			self.pos = pos

	class Move:
		def __init__(self, piece, to_pos, captured_piece):
			""" Creates a move of the given piece and to the given position, with the captured piece if it exists.
			
				piece -- a Piece object
				to_pos -- a tuple
				captured_piece -- a Piece object
			"""
			self.piece = piece
			from_pos = piece.pos
			self.from_pos = from_pos
			self.to_pos = to_pos
			self.captured_piece = captured_piece